use chrono::{DateTime, Duration, Utc};
use clap::Parser;
use fsrs::{
    current_retrievability, FSRSItem, FSRSReview, DEFAULT_PARAMETERS, FSRS, FSRS6_DEFAULT_DECAY,
};
use serde::{Deserialize, Serialize};
use std::collections::BTreeMap;
use std::fs;
use std::path::Path;

use serde::de::Deserializer;

#[derive(Debug, Clone)]
struct LogEntry {
    time: String,
    rating: u32, // 1=again, 2=hard, 3=good, 4=easy
}

impl<'de> Deserialize<'de> for LogEntry {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: Deserializer<'de>,
    {
        #[derive(Deserialize)]
        struct Helper {
            time: String,
            #[serde(default)]
            ok: Option<bool>,
            #[serde(default)]
            rating: Option<u32>,
        }

        let helper = Helper::deserialize(deserializer)?;

        let rating = if let Some(rating) = helper.rating {
            rating
        } else if let Some(ok) = helper.ok {
            if ok {
                3
            } else {
                1
            } // Convert old format: true->3 (good), false->1 (again)
        } else {
            3 // Default to "good"
        };

        Ok(LogEntry {
            time: helper.time,
            rating,
        })
    }
}

impl Serialize for LogEntry {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        use serde::Serialize;

        #[derive(Serialize)]
        struct Helper {
            time: String,
            rating: u32,
        }

        Helper {
            time: self.time.clone(),
            rating: self.rating,
        }
        .serialize(serializer)
    }
}

type LogEntries = Vec<LogEntry>;

const STATS_HTML_PATH: &str = "/tmp/anki-stats.html";

#[derive(Debug, PartialEq)]
struct DailyStats {
    date: String,
    reviews: usize,
    unique_items: usize,
}

#[derive(Debug, Clone, Deserialize, Serialize)]
struct Database {
    #[serde(flatten)]
    items: BTreeMap<String, LogEntries>,
}

impl Database {
    fn new() -> Self {
        Database {
            items: BTreeMap::new(),
        }
    }

    fn load(path: &Path) -> Result<Self, Box<dyn std::error::Error>> {
        if !path.exists() {
            return Ok(Database::new());
        }

        let content = fs::read_to_string(path)?;
        let database: Database = serde_json::from_str(&content)?;
        Ok(database)
    }

    fn save(&self, path: &Path) -> Result<(), Box<dyn std::error::Error>> {
        let content = serde_json::to_string_pretty(self)?;
        fs::write(path, content)?;
        Ok(())
    }

    fn insert_log(
        &mut self,
        item_uuid: &str,
        rating: u32,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let now = Utc::now();
        let new_log = LogEntry {
            time: now.to_rfc3339(),
            rating,
        };

        self.items
            .entry(item_uuid.to_string())
            .or_default()
            .insert(0, new_log);
        Ok(())
    }

    fn check_uuid_exists(&self, item_uuid: &str) -> bool {
        self.items.contains_key(item_uuid)
    }

    fn delete_item(&mut self, item_uuid: &str) -> bool {
        self.items.remove(item_uuid).is_some()
    }
}

#[derive(Parser)]
#[command(name = "anki-fsrs")]
#[command(about = "Anki-like spaced repetition system using FSRS")]
struct Args {
    #[arg(long = "due")]
    /// List all items that need to be reviewed
    due: bool,

    #[arg(long = "insert", num_args = 2)]
    /// Insert a learning record (UUID and rating: Again/[A], Hard/[H], Good/[G], Easy/[E])
    insert: Option<Vec<String>>,

    #[arg(long = "check")]
    /// Check if a UUID exists in the database
    check: Option<String>,

    #[arg(long = "when", num_args = 1)]
    /// Query when a card needs to be reviewed (takes UUID as argument)
    when: Option<String>,

    #[arg(long = "delete", num_args = 1)]
    /// Delete a card from the database (takes UUID as argument)
    delete: Option<String>,

    #[arg(long = "inspect", num_args = 1)]
    /// Inspect the review history for a specific UUID (takes UUID as argument)
    inspect: Option<String>,

    #[arg(long = "stats")]
    /// Statistics: Show number of reviews and unique items per day
    stats: bool,

    #[arg(long = "recent-decks")]
    /// List decks that were newly added in the past week
    recent_decks: bool,
}

fn collect_daily_stats(db: &Database) -> Result<Vec<DailyStats>, Box<dyn std::error::Error>> {
    let mut daily_reviews = BTreeMap::new();
    let mut daily_unique_items = BTreeMap::new();

    for (uuid, log_entries) in &db.items {
        for log in log_entries {
            let log_time = parse_datetime(&log.time)?;
            let date = log_time.format("%Y-%m-%d").to_string();

            *daily_reviews.entry(date.clone()).or_insert(0) += 1;
            daily_unique_items
                .entry(date)
                .or_insert_with(std::collections::HashSet::new)
                .insert(uuid);
        }
    }

    Ok(daily_reviews
        .into_iter()
        .map(|(date, reviews)| DailyStats {
            unique_items: daily_unique_items.get(&date).unwrap().len(),
            date,
            reviews,
        })
        .collect())
}

fn render_stats_html(daily_stats: &[DailyStats], total_cards: usize) -> String {
    let total_reviews: usize = daily_stats.iter().map(|day| day.reviews).sum();
    let active_days = daily_stats.len();
    let max_value = daily_stats
        .iter()
        .map(|day| day.reviews.max(day.unique_items))
        .max()
        .unwrap_or(1);
    let generated_at = Utc::now().format("%Y-%m-%d %H:%M UTC");

    let mut html = String::from(
        r#"<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Anki review statistics</title>
<style>
:root { color-scheme: light dark; font-family: Inter, ui-sans-serif, system-ui, sans-serif; }
* { box-sizing: border-box; }
body { margin: 0; background: #0f172a; color: #e2e8f0; }
main { width: min(1000px, calc(100% - 32px)); margin: 48px auto; }
h1 { margin: 0; font-size: clamp(28px, 5vw, 44px); letter-spacing: -0.04em; }
.subtitle { margin: 10px 0 28px; color: #94a3b8; }
.summary { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px, 1fr)); gap: 14px; }
.card, .chart { border: 1px solid #334155; border-radius: 16px; background: #111c31; box-shadow: 0 18px 45px #02061755; }
.card { padding: 20px; }
.card span { display: block; color: #94a3b8; font-size: 13px; text-transform: uppercase; letter-spacing: .08em; }
.card strong { display: block; margin-top: 8px; color: #f8fafc; font-size: 32px; }
.chart { margin-top: 20px; padding: 24px; overflow: hidden; }
.chart-header { display: flex; align-items: baseline; justify-content: space-between; gap: 16px; margin-bottom: 22px; }
.chart-header h2 { margin: 0; font-size: 20px; }
.legend { display: flex; gap: 16px; color: #94a3b8; font-size: 13px; }
.legend span::before { content: ""; display: inline-block; width: 9px; height: 9px; margin-right: 6px; border-radius: 3px; }
.legend .reviews::before { background: #38bdf8; }
.legend .cards::before { background: #a78bfa; }
.rows { display: grid; gap: 16px; }
.day { display: grid; grid-template-columns: 92px 1fr; align-items: center; gap: 14px; }
.date { color: #cbd5e1; font: 12px ui-monospace, SFMono-Regular, Menlo, monospace; }
.bars { display: grid; gap: 5px; }
.track { height: 22px; border-radius: 7px; background: #1e293b; overflow: hidden; }
.bar { display: flex; align-items: center; height: 100%; min-width: 42px; padding: 0 8px; border-radius: 7px; color: #07111f; font-size: 12px; font-weight: 700; white-space: nowrap; }
.bar.reviews { background: linear-gradient(90deg, #0284c7, #38bdf8); }
.bar.cards { background: linear-gradient(90deg, #7c3aed, #a78bfa); }
.empty { padding: 44px 0; color: #94a3b8; text-align: center; }
footer { margin: 18px 4px 0; color: #64748b; font-size: 12px; }
@media (max-width: 560px) { main { margin: 24px auto; } .chart { padding: 18px; } .chart-header { align-items: flex-start; flex-direction: column; } .day { grid-template-columns: 1fr; gap: 6px; } }
</style>
</head>
<body>
<main>
<h1>Anki review statistics</h1>
"#,
    );

    html.push_str(&format!(
        "<p class=\"subtitle\">Generated from the local review database on {generated_at}</p>\n\
         <section class=\"summary\">\n\
         <div class=\"card\"><span>Total reviews</span><strong>{total_reviews}</strong></div>\n\
         <div class=\"card\"><span>Total cards</span><strong>{total_cards}</strong></div>\n\
         <div class=\"card\"><span>Active days</span><strong>{active_days}</strong></div>\n\
         </section>\n\
         <section class=\"chart\">\n\
         <div class=\"chart-header\"><h2>Daily activity</h2><div class=\"legend\"><span class=\"reviews\">Reviews</span><span class=\"cards\">Unique cards</span></div></div>\n"
    ));

    if daily_stats.is_empty() {
        html.push_str("<div class=\"empty\">No review history yet.</div>\n");
    } else {
        html.push_str("<div class=\"rows\">\n");
        for day in daily_stats.iter().rev() {
            let review_width = (day.reviews * 100 / max_value).max(2);
            let card_width = (day.unique_items * 100 / max_value).max(2);
            html.push_str(&format!(
                "<div class=\"day\"><div class=\"date\">{}</div><div class=\"bars\">\
                 <div class=\"track\"><div class=\"bar reviews\" style=\"width: {}%\">{} reviews</div></div>\
                 <div class=\"track\"><div class=\"bar cards\" style=\"width: {}%\">{} cards</div></div>\
                 </div></div>\n",
                day.date, review_width, day.reviews, card_width, day.unique_items
            ));
        }
        html.push_str("</div>\n");
    }

    html.push_str("</section><footer>Generated by anki-fsrs</footer></main></body></html>\n");
    html
}

fn write_stats_html(
    path: &Path,
    daily_stats: &[DailyStats],
    total_cards: usize,
) -> Result<(), Box<dyn std::error::Error>> {
    fs::write(path, render_stats_html(daily_stats, total_cards))?;
    Ok(())
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();
    let current_dir = std::env::current_dir()?;
    let database_path = current_dir.join("/home/martins3/data/vn/docs/blog/anki.json");

    let mut db = Database::load(&database_path)?;

    if args.stats {
        let daily_stats = collect_daily_stats(&db)?;

        // Print the statistics
        println!("Daily review statistics:");
        println!("------------------------");
        for day in &daily_stats {
            println!(
                "{}: {} reviews, {} unique items",
                day.date, day.reviews, day.unique_items
            );
        }
        write_stats_html(Path::new(STATS_HTML_PATH), &daily_stats, db.items.len())?;
        println!("HTML report: {}", STATS_HTML_PATH);
    } else if args.due {
        // Use FSRS to determine which items are due for review
        let fsrs = FSRS::new(Some(&DEFAULT_PARAMETERS))?;

        let mut due_items_with_scores = Vec::new();

        for (uuid, log_entries) in &db.items {
            if log_entries.is_empty() {
                continue; // Skip items with no log entries
            }

            // Convert log entries to FSRS item for this specific UUID
            let fsrs_item = convert_log_entries_to_fsrs_item(log_entries)?;

            if let Ok(memory_state) = fsrs.memory_state(fsrs_item, None) {
                // Calculate days since last review
                let last_review = &log_entries[0]; // Most recent review is first in the list
                let last_time = parse_datetime(&last_review.time)?;
                let now = Utc::now();
                let days_elapsed = (now - last_time).num_days() as u32;

                // Calculate current retrievability
                let retrievability =
                    current_retrievability(memory_state, days_elapsed as f32, FSRS6_DEFAULT_DECAY);

                // Calculate how overdue the card is based on FSRS scheduling
                // The FSRS algorithm determines when a card should be reviewed based on its memory state
                // We'll calculate the ideal review time and see how overdue it is
                let desired_retention = 0.9;
                let next_states = fsrs.next_states(Some(memory_state), desired_retention, 0)?; // Calculate intervals from current state
                let ideal_interval = next_states.good.interval; // Use "good" rating as standard interval

                // Calculate how overdue the card is (in days)
                let days_overdue = (days_elapsed as f32 - ideal_interval).max(0.0);

                // Create a score that combines retrievability and how overdue the card is
                // Lower retrievability and more overdue = higher priority (more due)
                let score = (1.0 - retrievability) + (days_overdue / 30.0); // Normalize days overdue

                due_items_with_scores.push((
                    uuid.clone(),
                    memory_state,
                    retrievability,
                    score,
                    days_overdue,
                ));
            }
        }

        // Sort by score (higher score = more due) to get the most important cards
        due_items_with_scores
            .sort_by(|a, b| b.3.partial_cmp(&a.3).unwrap_or(std::cmp::Ordering::Equal));

        // Print the top 10 cards that most need to be reviewed
        for (uuid, _memory_state, _retrievability, _score, _days_overdue) in
            due_items_with_scores.iter().take(10)
        {
            println!("{}", uuid);
        }
    } else if let Some(insert_args) = args.insert {
        if insert_args.len() != 2 {
            eprintln!("Error: --insert requires exactly 2 arguments (UUID and rating: Again/[A], Hard/[H], Good/[G], Easy/[E])");
            std::process::exit(1);
        }

        let uuid = &insert_args[0];
        let rating_str = &insert_args[1];
        // Parse rating as string (Again/[A], Hard/[H], Good/[G], Easy/[E])
        let rating = match rating_str.to_lowercase().as_str() {
            "again" | "a" | "1" => 1,
            "hard" | "h" | "2" => 2,
            "good" | "g" | "3" => 3,
            "easy" | "e" | "4" => 4,
            _ => {
                eprintln!("Error: rating must be Again/[A], Hard/[H], Good/[G], or Easy/[E]");
                std::process::exit(1);
            }
        };

        // Validate rating is between 1 and 4
        if !(1..=4).contains(&rating) {
            eprintln!("Error: rating must be Again/[A], Hard/[H], Good/[G], or Easy/[E]");
            std::process::exit(1);
        }

        db.insert_log(uuid, rating)?;
        println!(
            "Successfully inserted record: UUID={}, rating={}",
            uuid, rating
        );
        db.save(&database_path)?;
    } else if let Some(uuid) = args.check {
        if !db.check_uuid_exists(&uuid) {
            std::process::exit(1);
        }
    } else if let Some(uuid) = args.when {
        query_review_time(&mut db, &uuid)?;
    } else if let Some(uuid) = args.delete {
        if db.check_uuid_exists(&uuid) {
            db.delete_item(&uuid);
            println!("Successfully deleted card with UUID: {}", uuid);
            db.save(&database_path)?;
        } else {
            eprintln!(
                "Notice: card with UUID {} not found in database; nothing to delete",
                uuid
            );
        }
    } else if let Some(uuid) = args.inspect {
        inspect_review_history(&db, &uuid)?;
    } else if args.recent_decks {
        list_recent_decks(&db)?;
    } else {
        // Print help if no arguments are provided
        Args::parse_from(["anki-fsrs", "--help"]);
    }

    Ok(())
}

/// List decks that were newly added in the past week
fn list_recent_decks(db: &Database) -> Result<(), Box<dyn std::error::Error>> {
    let now = Utc::now();
    let one_week_ago = now - Duration::days(7);

    let mut recent_decks = Vec::new();

    for (uuid, log_entries) in &db.items {
        if log_entries.is_empty() {
            continue;
        }

        // Find the earliest review time (when the deck was first added)
        let earliest_time = log_entries
            .iter()
            .filter_map(|entry| parse_datetime(&entry.time).ok())
            .min();

        if let Some(earliest_time) = earliest_time {
            // Check if the deck was added in the past week
            if earliest_time >= one_week_ago {
                recent_decks.push((uuid.clone(), earliest_time));
            }
        }
    }

    // Sort by addition time (newest first)
    recent_decks.sort_by(|a, b| b.1.cmp(&a.1));

    // Print the results
    println!("Decks added in the past week:");
    println!("------------------------------");

    if recent_decks.is_empty() {
        println!("No new decks added in the past week.");
    } else {
        println!("Found {} deck(s) added in the past week:\n", recent_decks.len());
        for (uuid, added_time) in recent_decks {
            let date_str = added_time.format("%Y-%m-%d %H:%M").to_string();
            println!("{} (added on {})", uuid, date_str);
        }
    }

    Ok(())
}

/// Convert our log entries to an FSRS item
fn convert_log_entries_to_fsrs_item(
    log_entries: &LogEntries,
) -> Result<FSRSItem, Box<dyn std::error::Error>> {
    if log_entries.is_empty() {
        return Ok(FSRSItem { reviews: vec![] });
    }

    // Sort entries by time (oldest first)
    let mut sorted_entries = log_entries.clone();
    sorted_entries.sort_by(|a, b| {
        let time_a = parse_datetime(&a.time).unwrap();
        let time_b = parse_datetime(&b.time).unwrap();
        time_a.cmp(&time_b)
    });

    let mut reviews = Vec::new();
    let mut prev_time: Option<DateTime<Utc>> = None;

    for entry in sorted_entries {
        let current_time = parse_datetime(&entry.time)?;

        if let Some(prev) = prev_time {
            let delta_t = (current_time - prev).num_days() as u32;
            let rating = entry.rating;

            reviews.push(FSRSReview { rating, delta_t });
        } else {
            // First review has delta_t = 0, use the rating from the entry
            reviews.push(FSRSReview {
                rating: entry.rating,
                delta_t: 0,
            });
        }

        prev_time = Some(current_time);
    }

    Ok(FSRSItem { reviews })
}

/// Helper function to parse datetime strings
fn parse_datetime(time_str: &str) -> Result<DateTime<Utc>, Box<dyn std::error::Error>> {
    let parsed_time =
        if time_str.contains('Z') || time_str.contains('+') || time_str.contains(' ') {
            DateTime::parse_from_rfc3339(time_str)?.with_timezone(&Utc)
        } else {
            // Parse as naive datetime and assume UTC - handle various fractional second formats
            let naive = chrono::NaiveDateTime::parse_from_str(time_str, "%Y-%m-%dT%H:%M:%S%.6f") // microsecond format
                .or_else(|_| {
                    chrono::NaiveDateTime::parse_from_str(time_str, "%Y-%m-%dT%H:%M:%S%.5f")
                }) // 5 digits fractional
                .or_else(|_| {
                    chrono::NaiveDateTime::parse_from_str(time_str, "%Y-%m-%dT%H:%M:%S%.4f")
                }) // 4 digits fractional
                .or_else(|_| {
                    chrono::NaiveDateTime::parse_from_str(time_str, "%Y-%m-%dT%H:%M:%S%.3f")
                }) // millisecond format
                .or_else(|_| {
                    chrono::NaiveDateTime::parse_from_str(time_str, "%Y-%m-%dT%H:%M:%S")
                })?; // no fractional seconds
            DateTime::<Utc>::from_naive_utc_and_offset(naive, Utc)
        };
    Ok(parsed_time)
}

/// Query when a specific card needs to be reviewed
fn query_review_time(db: &mut Database, uuid: &str) -> Result<(), Box<dyn std::error::Error>> {
    // Check if the card exists
    if !db.check_uuid_exists(uuid) {
        eprintln!(
            "Notice: card with UUID {} not found in database; skipping query",
            uuid
        );
        return Ok(());
    }

    // Get the log entries for this card
    let log_entries = &db.items[uuid];
    if log_entries.is_empty() {
        eprintln!("Card with UUID {} has no review history", uuid);
        return Ok(());
    }

    // Create FSRS instance
    let fsrs = FSRS::new(Some(&DEFAULT_PARAMETERS))?;

    // Convert log entries to FSRS item for this specific UUID
    let fsrs_item = convert_log_entries_to_fsrs_item(log_entries)?;

    // Calculate memory state
    let memory_state = fsrs.memory_state(fsrs_item, None)?;

    // Get the last review time
    let last_review = &log_entries[0]; // Most recent review is first in the list
    let last_time = parse_datetime(&last_review.time)?;
    let now = Utc::now();
    let days_elapsed = (now - last_time).num_days() as u32;

    // Calculate next states with a default desired retention (e.g., 0.9)
    let desired_retention = 0.9;
    let next_states = fsrs.next_states(Some(memory_state), desired_retention, days_elapsed)?;

    // Calculate due dates for each rating
    let again_due = now + Duration::days(next_states.again.interval.round().max(1.0) as i64);
    let hard_due = now + Duration::days(next_states.hard.interval.round().max(1.0) as i64);
    let good_due = now + Duration::days(next_states.good.interval.round().max(1.0) as i64);
    let easy_due = now + Duration::days(next_states.easy.interval.round().max(1.0) as i64);

    // Print the results
    println!("Card UUID: {}", uuid);
    println!("Last reviewed: {}", last_time);
    println!(
        "Current memory state - Stability: {:.2}, Difficulty: {:.2}",
        memory_state.stability, memory_state.difficulty
    );
    println!(
        "Current retrievability: {:.2}%",
        current_retrievability(memory_state, days_elapsed as f32, FSRS6_DEFAULT_DECAY) * 100.0
    );
    println!();
    println!("Next review times based on rating:");
    println!(
        "  Again:  {} (in {:.1} days)",
        again_due,
        next_states.again.interval.round().max(1.0)
    );
    println!(
        "  Hard:   {} (in {:.1} days)",
        hard_due,
        next_states.hard.interval.round().max(1.0)
    );
    println!(
        "  Good:   {} (in {:.1} days)",
        good_due,
        next_states.good.interval.round().max(1.0)
    );
    println!(
        "  Easy:   {} (in {:.1} days)",
        easy_due,
        next_states.easy.interval.round().max(1.0)
    );

    Ok(())
}

/// Inspect the review history for a specific UUID (read-only)
fn inspect_review_history(db: &Database, uuid: &str) -> Result<(), Box<dyn std::error::Error>> {
    // Check if the card exists
    if !db.check_uuid_exists(uuid) {
        eprintln!(
            "Notice: card with UUID {} not found in database; skipping inspection",
            uuid
        );
        return Ok(());
    }

    // Get the log entries for this card
    let log_entries = &db.items[uuid];
    if log_entries.is_empty() {
        eprintln!("Card with UUID {} has no review history", uuid);
        return Ok(());
    }

    // Print the card UUID
    println!("Card UUID: {}", uuid);
    println!("Review History:");
    println!("--------------");

    // Print each review in chronological order (oldest first)
    let mut sorted_entries = log_entries.clone();
    sorted_entries.sort_by(|a, b| {
        let time_a = parse_datetime(&a.time).unwrap();
        let time_b = parse_datetime(&b.time).unwrap();
        time_a.cmp(&time_b)
    });

    for (index, entry) in sorted_entries.iter().enumerate() {
        let rating_text = match entry.rating {
            1 => "Again",
            2 => "Hard",
            3 => "Good",
            4 => "Easy",
            _ => "Unknown",
        };

        // Format time to show only the date part (YYYY-MM-DD)
        let time = parse_datetime(&entry.time)?;
        let date_only = time.format("%Y-%m-%d").to_string();
        println!("  {}: {} - Rating: {} ({})", index + 1, date_only, rating_text, entry.rating);
    }

    // Show the most recent review
    if let Some(latest) = log_entries.first() {
        let rating_text = match latest.rating {
            1 => "Again",
            2 => "Hard",
            3 => "Good",
            4 => "Easy",
            _ => "Unknown",
        };
        let time = parse_datetime(&latest.time)?;
        let date_only = time.format("%Y-%m-%d").to_string();
        println!("\nMost Recent Review: {} - Rating: {} ({})", date_only, rating_text, latest.rating);
    }

    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn daily_stats_count_reviews_and_unique_cards() {
        let mut db = Database::new();
        db.items.insert(
            "card-one".to_string(),
            vec![
                LogEntry {
                    time: "2026-08-17T10:00:00Z".to_string(),
                    rating: 3,
                },
                LogEntry {
                    time: "2026-08-17T09:00:00Z".to_string(),
                    rating: 4,
                },
            ],
        );
        db.items.insert(
            "card-two".to_string(),
            vec![LogEntry {
                time: "2026-08-17T11:00:00Z".to_string(),
                rating: 2,
            }],
        );

        let stats = collect_daily_stats(&db).unwrap();

        assert_eq!(
            stats,
            vec![DailyStats {
                date: "2026-08-17".to_string(),
                reviews: 3,
                unique_items: 2,
            }]
        );
    }

    #[test]
    fn html_report_contains_summary_and_chart_data() {
        let html = render_stats_html(
            &[DailyStats {
                date: "2026-08-17".to_string(),
                reviews: 3,
                unique_items: 2,
            }],
            7,
        );

        assert!(html.contains("<strong>3</strong>"));
        assert!(html.contains("<strong>7</strong>"));
        assert!(html.contains("2026-08-17"));
        assert!(html.contains("3 reviews"));
        assert!(html.contains("2 cards"));
    }
}
