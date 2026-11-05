#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
#
# SPDX-License-Identifier: GPL-3.0-or-later

"""Isolated integration smoke tests; no desktop, user bus or login1 access.

Usage: python3 tests/test_demo_dbus.py --binary /absolute/path/deepin-screensaver
Requires gdbus, dbus-run-session and an installed Python with dbus + gi (the
runner probes /usr/bin/python3 too). No packages are installed. Only the mock
uses those bindings; the test runner itself uses Python's standard library.
Default: X11/offscreen. Add --session-type wayland only when main honors an
explicit QT_QPA_PLATFORM=offscreen; this tests unsupported-QPA safe failure.
Optional: --session-type wayland --compositor /path/to/waylandsaver-smoke
also checks password/grace/preview policy against fake input and mock login1.
Some UOS security policies deny signals to dbus-daemon even from its parent;
if so dbus-run-session cannot reap it and privileged manual cleanup is needed.
"""

import argparse
import os
import select
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from xml.sax.saxutils import escape

SERVICE = "com.deepin.ScreenSaver"
OBJECT = "/com/deepin/ScreenSaver"
PROPERTIES = "org.freedesktop.DBus.Properties"
POWER = "org.deepin.dde.Power1"
POWER_PATH = "/org/deepin/dde/Power1"


def call(destination, path, method, *args):
    result = subprocess.run(
        ["gdbus", "call", "--session", "--dest", destination,
         "--object-path", path, "--method", method, "--timeout", "10", *args],
        text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=12,
    )
    if result.returncode:
        raise AssertionError(f"{method}: {result.stderr.strip()}")
    return result.stdout.strip()


def has_owner(name):
    return call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                "org.freedesktop.DBus.NameHasOwner", name) == "(true,)"


def mock_power():
    import dbus
    import dbus.service
    from dbus.mainloop.glib import DBusGMainLoop
    from gi.repository import GLib

    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()

    class Power(dbus.service.Object):
        def __init__(self, path):
            super().__init__(bus, path)
            self.values = {"BatteryScreensaverDelay": dbus.Int32(123),
                           "LinePowerScreensaverDelay": dbus.Int32(456)}

        @dbus.service.method(PROPERTIES, in_signature="ss", out_signature="v")
        def Get(self, interface, name):
            if interface not in (POWER, "com.deepin.daemon.Power") or name not in self.values:
                raise dbus.exceptions.DBusException(
                    "Unknown power property", name="org.freedesktop.DBus.Error.InvalidArgs")
            return self.values[name]

        @dbus.service.method(PROPERTIES, in_signature="ssv", out_signature="")
        def Set(self, interface, name, value):
            self.Get(interface, name)
            self.values[name] = dbus.Int32(value)
            self.PropertiesChanged(interface, {name: self.values[name]}, [])

        @dbus.service.signal(PROPERTIES, signature="sa{sv}as")
        def PropertiesChanged(self, interface, changed, invalidated):
            pass

    def running_changed(running):
        if bool(running):
            (Path(os.environ["XDG_RUNTIME_DIR"]) / "unexpected-running").touch()

    def properties_changed(interface, changed, invalidated):
        if interface == SERVICE and "isRunning" in changed:
            running_changed(changed["isRunning"])

    bus.add_signal_receiver(running_changed, signal_name="isRunningChanged",
                            dbus_interface=SERVICE, path=OBJECT)
    bus.add_signal_receiver(properties_changed, signal_name="PropertiesChanged",
                            dbus_interface=PROPERTIES, path=OBJECT)
    # Keep both names/objects alive; the application selects by OS major version.
    names = [dbus.service.BusName(name, bus) for name in (POWER, "com.deepin.daemon.Power")]
    objects = [Power(path) for path in (POWER_PATH, "/com/deepin/daemon/Power")]
    if os.environ.get("DEMO_COMPOSITOR"):
        class Login(dbus.service.Object):
            @dbus.service.method("org.freedesktop.login1.Manager", in_signature="s", out_signature="",
                                 async_callbacks=("reply", "error"))
            def LockSession(self, session, reply, error):
                assert session == "demo"
                self.record_lock(reply)

            @dbus.service.method("org.freedesktop.login1.Manager", in_signature="u", out_signature="o")
            def GetSessionByPID(self, pid):
                assert pid > 0
                return "/org/freedesktop/login1/session/test"

            @dbus.service.method("org.freedesktop.login1.Session", in_signature="", out_signature="",
                                 async_callbacks=("reply", "error"))
            def Lock(self, reply, error):
                self.record_lock(reply)

            def record_lock(self, reply):
                root = Path(os.environ["XDG_RUNTIME_DIR"])
                with open(root / "locks", "a") as log:
                    log.write("lock\n")
                def finish():
                    reply()
                    (root / "lock-replied").touch()
                    return False
                GLib.timeout_add(1000, finish)

        names.append(dbus.service.BusName("org.freedesktop.login1", bus))
        objects.append(Login(bus, "/org/freedesktop/login1"))
        objects.append(Login(bus, "/org/freedesktop/login1/session/test"))
    GLib.MainLoop().run()


class DemoDBus(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        root = os.environ.get("DEMO_DBUS_TEST_ROOT")
        address = os.environ.get("DBUS_SESSION_BUS_ADDRESS", "")
        if not root or not address.startswith("unix:path=" + root + "/"):
            raise unittest.SkipTest("run this script with --binary to create a private bus")
        cls.log = open(Path(os.environ["XDG_RUNTIME_DIR"]) / "mock.log", "w+")
        cls.mock = subprocess.Popen(
            [os.environ["DEMO_DBUS_PYTHON"], str(Path(__file__).resolve()), "--mock"],
            stdout=cls.log, stderr=subprocess.STDOUT,
        )
        cls.addClassCleanup(cls.cleanup_mock)
        deadline = time.monotonic() + 5
        while not has_owner(POWER):
            if cls.mock.poll() is not None or time.monotonic() >= deadline:
                cls.log.seek(0)
                raise AssertionError("Power mock did not start: " + cls.log.read())
            time.sleep(0.05)
        if has_owner(SERVICE):
            raise AssertionError("Screensaver unexpectedly owned before activation")
        reply = call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                     "org.freedesktop.DBus.StartServiceByName", SERVICE, "0")
        if reply != "(uint32 1,)":
            raise AssertionError("Expected fresh D-Bus activation: " + reply)
        # Service ownership precedes object registration in main().
        deadline = time.monotonic() + 5
        while True:
            try:
                cls.get("isRunning")
                break
            except AssertionError:
                if time.monotonic() >= deadline:
                    raise
                time.sleep(0.05)

    @classmethod
    def cleanup_mock(cls):
        if cls.mock.poll() is None:
            cls.mock.terminate()
            try:
                cls.mock.wait(timeout=3)
            except subprocess.TimeoutExpired:
                cls.mock.kill()
                cls.mock.wait()
        cls.log.close()

    @staticmethod
    def get(name):
        return call(SERVICE, OBJECT, PROPERTIES + ".Get", SERVICE, name)

    @staticmethod
    def set(name, value):
        return call(SERVICE, OBJECT, PROPERTIES + ".Set", SERVICE, name, value)

    def test_activation_registration_and_get_all(self):
        self.assertTrue(has_owner(SERVICE))
        values = call(SERVICE, OBJECT, PROPERTIES + ".GetAll", SERVICE)
        for name in ("allScreenSaver", "currentScreenSaver", "isRunning",
                     "lockScreenAtAwake", "lockScreenDelay",
                     "batteryScreenSaverTimeout", "linePowerScreenSaverTimeout"):
            self.assertIn("'" + name + "':", values)
        self.assertEqual(self.get("isRunning"), "(<false>,)")
        if os.environ["XDG_SESSION_TYPE"] == "wayland":
            self.assertEqual(self.get("allScreenSaver"), "(<['deepin-custom-screensaver']>,)")
            self.assertEqual(self.get("currentScreenSaver"), "(<'deepin-custom-screensaver'>,)")

    def test_failed_start_does_not_report_running(self):
        missing = "__demo_dbus_nonexistent_saver__"
        # Unknown module must be rejected before requesting any lock.
        self.assertEqual(call(SERVICE, OBJECT, SERVICE + ".Preview", missing, "1"), "(false,)")
        self.assertEqual(self.get("isRunning"), "(<false>,)")
        if os.environ.get("DEMO_COMPOSITOR"):
            # Wayland rejects an unsupported currentScreenSaver assignment, so
            # Start would intentionally start the existing valid slideshow.
            time.sleep(0.1)
            self.assertFalse((Path(os.environ["XDG_RUNTIME_DIR"]) / "unexpected-running").exists())
            return
        original = self.get("currentScreenSaver")
        try:
            self.set("currentScreenSaver", "<'" + missing + "'>")
            self.assertEqual(call(SERVICE, OBJECT, SERVICE + ".Start"), "()")
            # Keep observing beyond the method reply: failure is asynchronous.
            deadline = time.monotonic() + 0.3
            while time.monotonic() < deadline:
                self.assertEqual(self.get("isRunning"), "(<false>,)")
                time.sleep(0.02)
        finally:
            # Get returns a tuple containing a GVariant, suitable for Set.
            self.set("currentScreenSaver", original[1:-2])
        # Give the independent signal receiver time to process queued signals.
        time.sleep(0.1)
        self.assertFalse((Path(os.environ["XDG_RUNTIME_DIR"]) / "unexpected-running").exists(),
                         "failed start emitted isRunning=true")

    @unittest.skipUnless(os.environ.get("DEMO_COMPOSITOR"), "optional fake compositor not supplied")
    def test_wayland_wake_lock_policy(self):
        log = Path(os.environ["XDG_RUNTIME_DIR"]) / "locks"
        for awake, delay, preview, locks in (
            (True, 0, False, 1), (True, 10, False, 1),
            (True, 0, True, 1), (False, 0, False, 1),
        ):
            with self.subTest(awake=awake, delay=delay, preview=preview):
                self.set("lockScreenAtAwake", "<" + str(awake).lower() + ">")
                self.set("lockScreenDelay", f"<int32 {delay}>")
                if preview:
                    self.assertEqual(call(SERVICE, OBJECT, SERVICE + ".Preview",
                                          "deepin-custom-screensaver", "1"), "(true,)")
                else:
                    call(SERVICE, OBJECT, SERVICE + ".Start")
                # Preview=true means accepted, not that locked already arrived.
                deadline = time.monotonic() + 5
                while self.get("isRunning") != "(<true>,)":
                    if time.monotonic() >= deadline:
                        self.fail("accepted Wayland start never activated")
                    time.sleep(0.02)
                deadline = time.monotonic() + 5
                while self.get("isRunning") != "(<false>,)":
                    if time.monotonic() >= deadline:
                        self.fail("fake compositor wake did not stop screensaver")
                    time.sleep(0.05)
                if awake and delay == 0 and not preview:
                    replied = log.parent / "lock-replied"
                    deadline = time.monotonic() + 5
                    while not log.exists():
                        if time.monotonic() >= deadline:
                            self.fail("login1 lock request never arrived")
                        time.sleep(0.02)
                    self.assertFalse(replied.exists(), "mock replied before pending check")
                    self.assertEqual(call(SERVICE, OBJECT, SERVICE + ".Preview",
                                          "deepin-custom-screensaver", "1"), "(false,)")
                    while not replied.exists():
                        if time.monotonic() >= deadline:
                            self.fail("delayed login1 reply never arrived")
                        time.sleep(0.02)
                time.sleep(0.15)
                self.assertEqual(len(log.read_text().splitlines()) if log.exists() else 0, locks)

    def test_lock_settings_round_trip(self):
        for value in ("true", "false"):
            self.set("lockScreenAtAwake", "<" + value + ">")
            self.assertEqual(self.get("lockScreenAtAwake"), "(<" + value + ">,)")
        for value in (7, 1):
            self.set("lockScreenDelay", f"<int32 {value}>")
            self.assertEqual(self.get("lockScreenDelay"), f"(<{value}>,)")

    def test_power_timeout_property_mapping(self):
        for property_name, power_name, initial, changed in (
            ("batteryScreenSaverTimeout", "BatteryScreensaverDelay", 123, 321),
            ("linePowerScreenSaverTimeout", "LinePowerScreensaverDelay", 456, 654),
        ):
            with self.subTest(property=property_name):
                self.assertEqual(self.get(property_name), f"(<{initial}>,)")
                self.set(property_name, f"<int32 {changed}>")
                # Set is asynchronous in the application; allow its delivery.
                deadline = time.monotonic() + 3
                while self.get(property_name) != f"(<{changed}>,)":
                    if time.monotonic() >= deadline:
                        self.fail(f"{power_name} was not updated")
                    time.sleep(0.05)
                # Assert the actual Power1 property, not merely a local cache.
                # Legacy OS releases use a separate object with the same values.
                modern = call(POWER, POWER_PATH, PROPERTIES + ".Get", POWER, power_name)
                legacy = call("com.deepin.daemon.Power", "/com/deepin/daemon/Power",
                              PROPERTIES + ".Get", "com.deepin.daemon.Power", power_name)
                self.assertIn(f"(<{changed}>,)", (modern, legacy))


def main():
    if "--mock" in sys.argv:
        mock_power()
        return 0
    if "--inside" in sys.argv:
        if not os.environ.get("DBUS_SESSION_BUS_ADDRESS"):
            raise SystemExit("No private bus address")
        server = None
        server_log = None
        try:
            if os.environ.get("DEMO_COMPOSITOR"):
                # QPA connects during service construction. Publish the fake
                # socket before D-Bus activates the GUI, never patch it later.
                server_log = open(Path(os.environ["XDG_RUNTIME_DIR"]) / "compositor.log", "w")
                server = subprocess.Popen([os.environ["DEMO_COMPOSITOR"], "--serve", "1500"],
                                          stdout=subprocess.PIPE, stderr=server_log, text=True)
                if not select.select([server.stdout], [], [], 5)[0]:
                    raise RuntimeError("fake compositor did not publish socket")
                socket = server.stdout.readline().strip()
                if not socket or "/" in socket:
                    raise RuntimeError("invalid fake compositor socket: " + socket)
                os.environ.update(WAYLAND_DISPLAY=socket, QT_QPA_PLATFORM="wayland",
                                  QT_WAYLAND_DISABLE_WINDOWDECORATION="1",
                                  DBUS_SYSTEM_BUS_ADDRESS=os.environ["DBUS_SESSION_BUS_ADDRESS"])
                values = {key: os.environ[key] for key in (
                    "WAYLAND_DISPLAY", "QT_QPA_PLATFORM", "QT_WAYLAND_DISABLE_WINDOWDECORATION",
                    "DBUS_SYSTEM_BUS_ADDRESS")}
                call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                     "org.freedesktop.DBus.UpdateActivationEnvironment", repr(values))
            unittest.main(argv=[sys.argv[0], "-v"])
            return 0
        finally:
            if server is not None:
                server.terminate()
                try:
                    server.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    server.kill()
                    server.wait()
                server.stdout.close()
            if server_log is not None:
                server_log.close()

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", required=True, type=Path, help="built deepin-screensaver executable")
    parser.add_argument("--session-type", choices=("x11", "wayland"), default="x11")
    parser.add_argument("--compositor", type=Path, help="optional fake waylandsaver smoke executable")
    parser.add_argument("--session-fallback", action="store_true", help="exercise login1 GetSessionByPID instead of XDG_SESSION_ID")
    args = parser.parse_args()
    if args.compositor and args.session_type != "wayland":
        parser.error("--compositor requires --session-type wayland")
    if args.compositor and not os.access(args.compositor.resolve(), os.X_OK):
        parser.error("--compositor must be executable")
    binary = args.binary.resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        parser.error(f"not an executable: {binary}")
    for tool in ("gdbus", "dbus-run-session"):
        if not shutil.which(tool):
            parser.error(f"missing command: {tool}")
    mock_python = None
    for candidate in dict.fromkeys((sys.executable, "/usr/bin/python3")):
        if subprocess.run([candidate, "-c", "import dbus, gi"],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0:
            mock_python = candidate
            break
    if mock_python is None:
        parser.error("mock requires installed dbus/gi bindings (also tried /usr/bin/python3)")

    with tempfile.TemporaryDirectory(prefix="screensaver-dbus-") as temporary:
        root = Path(temporary)
        env = os.environ.copy()
        for key in ("DBUS_SESSION_BUS_ADDRESS", "DBUS_STARTER_ADDRESS", "DBUS_STARTER_BUS_TYPE",
                    "DISPLAY", "WAYLAND_DISPLAY", "SESSION_MANAGER", "XDG_SESSION_ID"):
            env.pop(key, None)
        for key in ("HOME", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME",
                    "XDG_RUNTIME_DIR", "XDG_CONFIG_DIRS", "XDG_DATA_DIRS"):
            directory = root / key.lower()
            directory.mkdir(mode=0o700)
            env[key] = str(directory)
        env.update(QT_QPA_PLATFORM="offscreen", XDG_SESSION_TYPE=args.session_type,
                   WAYLAND_DISPLAY="nonexistent-demo-wayland-socket",
                   QT_QUICK_BACKEND="software", QT_OPENGL="software", QT_ACCESSIBILITY="0",
                   NO_AT_BRIDGE="1", DEMO_DBUS_PYTHON=mock_python, DEMO_DBUS_TEST_ROOT=str(root),
                   # Even accidental system-bus access cannot reach real login1/dconfig.
                   DBUS_SYSTEM_BUS_ADDRESS="unix:path=" + str(root / "no-system-bus"))
        if args.compositor:
            env.update(DEMO_COMPOSITOR=str(args.compositor.resolve()))
            if not args.session_fallback:
                env["XDG_SESSION_ID"] = "demo"
        services = root / "services"
        services.mkdir()
        # D-Bus Exec follows shell-style quoting, but is not executed via a shell.
        quoted_binary = '"' + str(binary).replace("\\", "\\\\").replace('"', '\\"') + '"'
        (services / (SERVICE + ".service")).write_text(
            f"[D-BUS Service]\nName={SERVICE}\nExec={quoted_binary} --dbus\n", encoding="utf-8")
        config = root / "bus.conf"
        # No standard_session_servicedirs: never activate installed desktop services.
        config.write_text(
            '<busconfig><type>session</type><listen>unix:tmpdir=' + escape(str(root)) + '</listen>'
            '<auth>EXTERNAL</auth><servicedir>' + escape(str(services)) + '</servicedir>'
            '<policy context="default"><allow send_destination="*"/>'
            '<allow receive_sender="*"/><allow own="*"/></policy></busconfig>', encoding="utf-8")
        process = subprocess.Popen(
            ["dbus-run-session", "--config-file=" + str(config), "--", sys.executable,
             str(Path(__file__).resolve()), "--inside"], env=env, start_new_session=True,
        )
        try:
            return process.wait(timeout=90)
        except subprocess.TimeoutExpired:
            print("FAIL: integration run exceeded 90 seconds", file=sys.stderr)
            return 1
        finally:
            # Include D-Bus-activated children, even after dbus-run-session exits.
            try:
                os.killpg(process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            except PermissionError as error:
                print(f"WARNING: private process-group cleanup denied: {error}", file=sys.stderr)
            process.wait()


if __name__ == "__main__":
    sys.exit(main())
