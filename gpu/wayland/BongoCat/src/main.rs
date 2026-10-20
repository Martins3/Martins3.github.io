mod art;
mod input;
mod state;

use calloop::{
    EventLoop,
    signals::{Signal, Signals},
};
use smithay_client_toolkit::{
    compositor::{CompositorHandler, CompositorState, Region},
    delegate_compositor, delegate_output, delegate_registry, delegate_shm, delegate_xdg_shell,
    delegate_xdg_window,
    output::{OutputHandler, OutputState},
    reexports::calloop_wayland_source::WaylandSource,
    registry::{ProvidesRegistryState, RegistryState},
    registry_handlers,
    shell::{
        WaylandSurface,
        xdg::{
            XdgShell,
            window::{Window, WindowConfigure, WindowDecorations, WindowHandler},
        },
    },
    shm::{Shm, ShmHandler},
};
use std::{
    path::PathBuf,
    time::{Duration, Instant},
};
use wayland_client::{
    Connection, QueueHandle,
    globals::registry_queue_init,
    protocol::{wl_buffer, wl_output, wl_surface},
};

type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;
const FRAME_INTERVAL: Duration = Duration::from_nanos(16_666_667);
const APP_ID: &str = "io.github.bongocat.Core";

struct Options {
    devices: Vec<PathBuf>,
    width: u32,
}

fn options(args: impl Iterator<Item = String>) -> Result<Option<Options>> {
    let mut args = args.peekable();
    let mut devices = Vec::new();
    let mut width = 306;
    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--help" | "-h" => {
                println!(
                    "bongocat --device PATH [--device PATH ...] [--width 306]\n\
                    bongocat --list-devices\n\n\
                    GNOME 50 Wayland; install the companion extension for placement.\n\
                    Keys and mouse buttons animate the cat. Ctrl+C stops it."
                );
                return Ok(None);
            }
            "--list-devices" => {
                input::list()?;
                return Ok(None);
            }
            "--device" => devices.push(PathBuf::from(args.next().ok_or("--device needs a path")?)),
            "--width" => {
                width = args.next().ok_or("--width needs a number")?.parse()?;
                if !(64..=612).contains(&width) {
                    return Err("--width must be between 64 and 612".into());
                }
            }
            _ => return Err(format!("unknown option: {arg}; use --help").into()),
        }
    }
    if devices.is_empty() {
        return Err("provide at least one --device PATH; use --list-devices".into());
    }
    // Reject aliases of the same event device, which would double count input.
    let mut canonical = std::collections::BTreeSet::new();
    for path in &devices {
        let path = std::fs::canonicalize(path).map_err(|e| format!("{}: {e}", path.display()))?;
        if !canonical.insert(path) {
            return Err("duplicate input device".into());
        }
    }
    Ok(Some(Options { devices, width }))
}

fn main() {
    if let Err(e) = start() {
        eprintln!("bongocat: {e}");
        std::process::exit(1);
    }
}

fn start() -> Result<()> {
    let Some(options) = options(std::env::args().skip(1))? else {
        return Ok(());
    };
    let (mut event_loop, mut app) = App::new(options)?;
    eprintln!("GNOME placement requires bongocat-core@local; see README. Ctrl+C to exit.");
    app.run(&mut event_loop)
}

struct App {
    registry: RegistryState,
    outputs: OutputState,
    shm: Shm,
    window: Window,
    qh: QueueHandle<Self>,
    input: input::Input,
    pose: state::Pose,
    frames: Option<art::Frames>,
    width: u32,
    height: u32,
    scale: i32,
    configured: bool,
    rebuild: bool,
    frame_pending: bool,
    shown: Option<state::Visual>,
    next_frame: Instant,
    exit: bool,
    commits: u64,
}

impl App {
    fn new(options: Options) -> Result<(EventLoop<'static, Self>, Self)> {
        let mut event_loop = EventLoop::try_new()?;
        let handle = event_loop.handle();
        handle.insert_source(
            Signals::new(&[Signal::SIGINT, Signal::SIGTERM])?,
            |_, _, app: &mut Self| app.exit = true,
        )?;
        let mut input = input::Input::new(options.devices.clone(), &handle)?;
        let mut pose = state::Pose::new(options.devices.len());
        input.connect(&handle, &mut pose, true)?;
        let conn =
            Connection::connect_to_env().map_err(|e| format!("cannot connect to Wayland: {e}"))?;
        let (globals, queue) = registry_queue_init(&conn)?;
        let qh = queue.handle();
        let compositor = CompositorState::bind(&globals, &qh)?;
        let shell = XdgShell::bind(&globals, &qh)?;
        let shm = Shm::bind(&globals, &qh)?;
        let surface = compositor.create_surface(&qh);
        let empty = Region::new(&compositor)?;
        surface.set_input_region(Some(empty.wl_region()));
        let window = shell.create_window(surface, WindowDecorations::ClientOnly, &qh);
        let width = options.width;
        let height = (width * 354 + 306) / 612;
        window.set_app_id(APP_ID);
        window.set_title("BongoCat");
        window.set_min_size(Some((width, height)));
        window.set_max_size(Some((width, height)));
        window.commit();
        WaylandSource::new(conn, queue).insert(handle)?;
        let mut app = Self {
            registry: RegistryState::new(&globals),
            outputs: OutputState::new(&globals, &qh),
            shm,
            window,
            qh,
            input,
            pose,
            frames: None,
            width,
            height,
            scale: 1,
            configured: false,
            rebuild: true,
            frame_pending: false,
            shown: None,
            next_frame: Instant::now(),
            exit: false,
            commits: 0,
        };
        // Process the initial registry state without introducing a periodic tick.
        event_loop.dispatch(Duration::ZERO, &mut app)?;
        Ok((event_loop, app))
    }

    fn run(&mut self, event_loop: &mut EventLoop<'static, Self>) -> Result<()> {
        while !self.exit {
            if self.input.rescan {
                self.input
                    .connect(&event_loop.handle(), &mut self.pose, false)?;
            }
            let now = Instant::now();
            let desired = self.pose.visual(now);
            let dirty = self.rebuild || self.shown != Some(desired);
            let ready = self.configured
                && !self.frame_pending
                && (self.rebuild
                    || self
                        .frames
                        .as_ref()
                        .is_some_and(|frames| frames.available()));
            if dirty && ready && now >= self.next_frame {
                self.present(desired)?;
            }
            let now = Instant::now();
            let mut deadline = self.pose.deadline(now);
            if dirty && ready && now < self.next_frame && !self.frame_pending {
                deadline = Some(deadline.map_or(self.next_frame, |d| d.min(self.next_frame)));
            }
            event_loop.dispatch(deadline.map(|d| d.saturating_duration_since(now)), self)?;
        }
        Ok(())
    }

    fn present(&mut self, desired: state::Visual) -> Result<()> {
        if self.rebuild {
            let w = self.width * self.scale as u32;
            let h = self.height * self.scale as u32;
            self.frames = Some(art::Frames::new(&self.shm, &self.qh, w, h)?);
            self.window.wl_surface().set_buffer_scale(self.scale);
            self.rebuild = false;
        }
        let surface = self.window.wl_surface();
        // Draw only into a released buffer; no buffer allocation or PNG decoding.
        let buffer = self.frames.as_mut().unwrap().draw(desired);
        surface.attach(Some(buffer), 0, 0);
        surface.damage_buffer(0, 0, i32::MAX, i32::MAX);
        surface.frame(&self.qh, surface.clone());
        self.window.commit();
        self.frame_pending = true;
        self.shown = Some(desired);
        let now = Instant::now();
        self.pose.presented(desired.keys, now);
        self.next_frame = now + FRAME_INTERVAL;
        self.commits += 1;
        Ok(())
    }
}

impl CompositorHandler for App {
    fn scale_factor_changed(
        &mut self,
        _: &Connection,
        _: &QueueHandle<Self>,
        _: &wl_surface::WlSurface,
        factor: i32,
    ) {
        if self.scale != factor {
            self.scale = factor.clamp(1, 4);
            self.rebuild = true;
        }
    }
    fn transform_changed(
        &mut self,
        _: &Connection,
        _: &QueueHandle<Self>,
        _: &wl_surface::WlSurface,
        _: wl_output::Transform,
    ) {
    }
    fn frame(&mut self, _: &Connection, _: &QueueHandle<Self>, _: &wl_surface::WlSurface, _: u32) {
        self.frame_pending = false;
    }
    fn surface_enter(
        &mut self,
        _: &Connection,
        _: &QueueHandle<Self>,
        _: &wl_surface::WlSurface,
        _: &wl_output::WlOutput,
    ) {
    }
    fn surface_leave(
        &mut self,
        _: &Connection,
        _: &QueueHandle<Self>,
        _: &wl_surface::WlSurface,
        _: &wl_output::WlOutput,
    ) {
    }
}
impl WindowHandler for App {
    fn request_close(&mut self, _: &Connection, _: &QueueHandle<Self>, _: &Window) {
        self.exit = true;
    }
    fn configure(
        &mut self,
        _: &Connection,
        _: &QueueHandle<Self>,
        _: &Window,
        _: WindowConfigure,
        _: u32,
    ) {
        self.configured = true;
    }
}
impl OutputHandler for App {
    fn output_state(&mut self) -> &mut OutputState {
        &mut self.outputs
    }
    fn new_output(&mut self, _: &Connection, _: &QueueHandle<Self>, _: wl_output::WlOutput) {}
    fn update_output(&mut self, _: &Connection, _: &QueueHandle<Self>, _: wl_output::WlOutput) {}
    fn output_destroyed(&mut self, _: &Connection, _: &QueueHandle<Self>, _: wl_output::WlOutput) {}
}
impl ShmHandler for App {
    fn shm_state(&mut self) -> &mut Shm {
        &mut self.shm
    }
}
impl ProvidesRegistryState for App {
    fn registry(&mut self) -> &mut RegistryState {
        &mut self.registry
    }
    registry_handlers![OutputState];
}
delegate_compositor!(App);
delegate_output!(App);
delegate_shm!(App);
delegate_xdg_shell!(App);
delegate_xdg_window!(App);
delegate_registry!(App);
impl wayland_client::Dispatch<wl_buffer::WlBuffer, ()> for App {
    fn event(
        state: &mut Self,
        buffer: &wl_buffer::WlBuffer,
        event: wl_buffer::Event,
        _: &(),
        _: &Connection,
        _: &QueueHandle<Self>,
    ) {
        if let wl_buffer::Event::Release = event {
            if let Some(frames) = state.frames.as_mut() {
                frames.release(buffer);
            }
        }
    }
}

#[cfg(test)]
mod integration {
    use super::*;
    use calloop::timer::{TimeoutAction, Timer};

    fn cpu_ticks() -> u64 {
        let stat = std::fs::read_to_string("/proc/self/stat").unwrap();
        let fields: Vec<_> = stat
            .rsplit_once(')')
            .unwrap()
            .1
            .split_whitespace()
            .collect();
        fields[11].parse::<u64>().unwrap() + fields[12].parse::<u64>().unwrap()
    }

    fn live(active: bool) {
        let (mut event_loop, mut app) = App::new(Options {
            devices: vec![],
            width: 306,
        })
        .unwrap();
        app.pose = state::Pose::new(1);
        if active {
            let mut phase = 0;
            event_loop
                .handle()
                .insert_source(Timer::immediate(), move |_, _, app| {
                    let now = Instant::now();
                    match phase {
                        0 => app.pose.key(0, 30, 1, now),
                        2 => app.pose.key(0, 30, 0, now),
                        5 => app.pose.key(0, 272, 1, now),
                        7 => app.pose.key(0, 272, 0, now),
                        _ => (),
                    }
                    app.pose.relative(0, if phase < 5 { 8 } else { -8 });
                    app.pose.relative(1, if phase % 2 == 0 { 8 } else { -8 });
                    phase = (phase + 1) % 10;
                    TimeoutAction::ToDuration(Duration::from_millis(10))
                })
                .unwrap();
        }
        event_loop
            .handle()
            .insert_source(
                Timer::from_duration(Duration::from_secs(60)),
                |_, _, app| {
                    app.exit = true;
                    TimeoutAction::Drop
                },
            )
            .unwrap();
        let ticks = cpu_ticks();
        let start = Instant::now();
        app.run(&mut event_loop).unwrap();
        let ticks = cpu_ticks() - ticks;
        let tick_rate = std::process::Command::new("getconf")
            .arg("CLK_TCK")
            .output()
            .unwrap();
        let hz: f64 = String::from_utf8(tick_rate.stdout)
            .unwrap()
            .trim()
            .parse()
            .unwrap();
        eprintln!(
            "active={active} commits={} elapsed={:.3}s cpu={:.4}%",
            app.commits,
            start.elapsed().as_secs_f64(),
            ticks as f64 / hz / start.elapsed().as_secs_f64() * 100.0
        );
        for line in std::fs::read_to_string("/proc/self/status")
            .unwrap()
            .lines()
        {
            if line.starts_with("VmRSS:")
                || line.starts_with("VmHWM:")
                || line.starts_with("Threads:")
            {
                eprintln!("{line}");
            }
        }
        if active {
            assert!(app.commits > 100 && app.commits <= 3601);
        } else {
            assert_eq!(app.commits, 1);
        }
    }

    #[test]
    #[ignore = "opens a Wayland window for 60 seconds"]
    fn live_wayland_idle() {
        live(false);
    }

    #[test]
    #[ignore = "opens a Wayland window for 60 seconds"]
    fn live_wayland_active() {
        live(true);
    }

    #[test]
    fn invalid_cli_options_fail() {
        for args in [
            vec![],
            vec!["--device"],
            vec!["--width", "0"],
            vec!["--width", "10000"],
            vec!["--wat"],
        ] {
            assert!(options(args.into_iter().map(str::to_owned)).is_err());
        }
    }
}
