use std::collections::BTreeSet;
use std::path::{Path, PathBuf};

use anyhow::Result;
use clap::{Parser, Subcommand};
use collei_balloond::config::Config;
use collei_balloond::controller;
use tracing_subscriber::EnvFilter;

#[derive(Debug, Parser)]
#[command(version, about = "Adaptive virtio-balloon controller for Collei VMs")]
struct Cli {
    #[arg(long, default_value = "~/.config/collei/balloond.toml")]
    config: PathBuf,

    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    Run {
        #[arg(long)]
        dry_run: bool,
        #[arg(long = "only")]
        only: Vec<String>,
    },
    Status {
        #[arg(long)]
        json: bool,
        #[arg(long = "only")]
        only: Vec<String>,
    },
    Restore {
        #[arg(long = "vm")]
        vm: Vec<String>,
    },
    CheckConfig,
}

#[tokio::main]
async fn main() -> Result<()> {
    tracing_subscriber::fmt()
        .with_env_filter(
            EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new("info")),
        )
        .with_target(false)
        .init();
    let cli = Cli::parse();
    let config = Config::load(Path::new(&cli.config))?;
    match cli.command {
        Command::Run { dry_run, only } => {
            controller::run_daemon(&config, &set(only), dry_run).await
        }
        Command::Status { json, only } => {
            let reports = controller::status(&config, &set(only)).await?;
            if json {
                println!("{}", serde_json::to_string_pretty(&reports)?);
            } else {
                print_status(&reports);
            }
            Ok(())
        }
        Command::Restore { vm } => {
            let results = controller::restore(&config, &set(vm)).await?;
            for result in results {
                println!(
                    "{}: {} MiB -> {} MiB (target {} MiB)",
                    result.name, result.before_mib, result.actual_mib, result.target_mib
                );
            }
            Ok(())
        }
        Command::CheckConfig => {
            println!("configuration is valid");
            Ok(())
        }
    }
}

fn set(values: Vec<String>) -> BTreeSet<String> {
    values.into_iter().collect()
}

fn print_status(reports: &[controller::VmReport]) {
    println!(
        "{:<45} {:>8} {:>10} {:>11} {:>10} {:>10} {:>10}  STATE",
        "VM", "PID", "CAP MiB", "CURRENT MiB", "RSS MiB", "AVAIL MiB", "RSV MiB"
    );
    for report in reports {
        println!(
            "{:<45} {:>8} {:>10} {:>11} {:>10} {:>10} {:>10}  {}",
            report.name,
            display(report.pid.map(u64::from)),
            display(report.capacity_mib),
            display(report.current_mib),
            display(report.rss_mib),
            display(report.available_mib),
            display(report.reserve_mib),
            report.reason
        );
    }
}

fn display(value: Option<u64>) -> String {
    value.map_or_else(|| "-".to_owned(), |number| number.to_string())
}
