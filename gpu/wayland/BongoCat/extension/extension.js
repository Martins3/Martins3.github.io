import GLib from 'gi://GLib';
import {Extension} from 'resource:///org/gnome/shell/extensions/extension.js';

const APP_ID = 'io.github.bongocat.Core';

export default class BongoCat extends Extension {
    enable() {
        this._windows = new Map();
        this._sources = new Set();
        this._signals = [];
        this._lastFocus = global.display.focus_window;
        this._connect(global.display, 'notify::focus-window', () => {
            const window = global.display.focus_window;
            if (window && window.get_wm_class() !== APP_ID)
                this._lastFocus = window;
        });
        this._connect(global.window_manager, 'map', (_wm, actor) => this._manage(actor.meta_window));
        this._connect(global.display, 'workareas-changed', () => this._placeAll());
        this._connect(global.backend.get_monitor_manager(), 'monitors-changed', () => this._placeAll());
        for (const actor of global.get_window_actors())
            this._manage(actor.meta_window);
    }

    _connect(object, signal, callback) {
        this._signals.push([object, object.connect(signal, callback)]);
    }

    _manage(window) {
        if (!window || window.get_wm_class() !== APP_ID || this._windows.has(window))
            return;
        const previous = {
            above: window.is_above(),
            sticky: window.is_on_all_workspaces(),
            hidden: window.skip_taskbar,
            rect: window.get_frame_rect(),
            sizeSignal: window.connect('size-changed', () => this._place(window)),
            goneSignal: window.connect('unmanaged', () => {
                const saved = this._windows.get(window);
                if (saved) {
                    window.disconnect(saved.sizeSignal);
                    window.disconnect(saved.goneSignal);
                    this._windows.delete(window);
                }
            }),
        };
        this._windows.set(window, previous);
        window.make_above();
        window.stick();
        if (!previous.hidden)
            window.hide_from_window_list();
        this._place(window);
        // Mapping can briefly focus an xdg_toplevel. Return focus only if
        // BongoCat still owns it, never undo a later user focus change.
        const source = GLib.idle_add(GLib.PRIORITY_DEFAULT_IDLE, () => {
            this._sources.delete(source);
            if (global.display.focus_window === window && this._lastFocus?.get_compositor_private())
                this._lastFocus.activate(global.get_current_time());
            return GLib.SOURCE_REMOVE;
        });
        this._sources.add(source);
    }

    _place(window) {
        const monitor = global.display.get_primary_monitor();
        if (monitor < 0)
            return;
        const area = global.workspace_manager.get_active_workspace().get_work_area_for_monitor(monitor);
        const rect = window.get_frame_rect();
        const x = area.x + Math.max(0, area.width - rect.width - 16);
        const y = area.y + Math.max(0, area.height - rect.height - 16);
        if (rect.x !== x || rect.y !== y)
            window.move_frame(false, x, y);
    }

    _placeAll() {
        for (const window of this._windows.keys())
            this._place(window);
    }

    disable() {
        for (const source of this._sources)
            GLib.Source.remove(source);
        for (const [object, signal] of this._signals)
            object.disconnect(signal);
        for (const [window, saved] of this._windows) {
            window.disconnect(saved.sizeSignal);
            window.disconnect(saved.goneSignal);
            if (!saved.above)
                window.unmake_above();
            if (!saved.sticky)
                window.unstick();
            if (!saved.hidden)
                window.show_in_window_list();
            window.move_frame(false, saved.rect.x, saved.rect.y);
        }
        this._windows.clear();
        this._sources.clear();
        this._signals = [];
        this._lastFocus = null;
    }
}
