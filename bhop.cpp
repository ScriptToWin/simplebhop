// Standalone external auto-bhop for CS2 (Linux)
// - Only active while CS2 process is running
// - Mouse4 (side button) = on/off toggle
// - Spams scroll-wheel-up (~28 ms) while ON (works in place / AFK too)
//
// Bind in CS2:  Mouse Wheel Up  →  +jump
// Build & run:  sudo bash run_bhop.sh
// Stop:         Ctrl+C

#include <chrono>
#include <csignal>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <iostream>
#include <linux/input.h>
#include <linux/uinput.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

volatile sig_atomic_t g_running = 1;
int g_uinput = -1;
bool g_bhop_on = false;

using clock = std::chrono::steady_clock;

void on_signal(int) { g_running = 0; }

// ---------- uinput (virtual scroll) ----------

void emit(std::uint16_t type, std::uint16_t code, int value) {
    if (g_uinput < 0) return;
    input_event ev{};
    gettimeofday(&ev.time, nullptr);
    ev.type = type;
    ev.code = code;
    ev.value = value;
    (void)::write(g_uinput, &ev, sizeof(ev));
}

void sync_report() { emit(EV_SYN, SYN_REPORT, 0); }

void wheel_up() {
    emit(EV_REL, REL_WHEEL, 1);
    sync_report();
}

bool open_uinput() {
    g_uinput = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (g_uinput < 0) {
        std::cerr << "Failed to open /dev/uinput (need root)\n";
        return false;
    }
    ioctl(g_uinput, UI_SET_EVBIT, EV_SYN);
    ioctl(g_uinput, UI_SET_EVBIT, EV_REL);
    ioctl(g_uinput, UI_SET_RELBIT, REL_WHEEL);

    uinput_setup setup{};
    setup.id.bustype = BUS_USB;
    setup.id.vendor  = 0x0451;
    setup.id.product = 0xe008;
    setup.id.version = 1;
    std::strncpy(setup.name, "standalone-bhop", UINPUT_MAX_NAME_SIZE - 1);

    if (ioctl(g_uinput, UI_DEV_SETUP, &setup) != 0 ||
        ioctl(g_uinput, UI_DEV_CREATE) != 0) {
        std::cerr << "Failed to create uinput device\n";
        ::close(g_uinput);
        g_uinput = -1;
        return false;
    }
    return true;
}

void close_uinput() {
    if (g_uinput >= 0) {
        ioctl(g_uinput, UI_DEV_DESTROY);
        ::close(g_uinput);
        g_uinput = -1;
    }
}

// ---------- CS2 process check ----------

bool cs2_running() {
    DIR* d = opendir("/proc");
    if (!d) return false;
    bool found = false;
    while (dirent* e = readdir(d)) {
        if (e->d_name[0] < '1' || e->d_name[0] > '9') continue;
        std::string path = std::string("/proc/") + e->d_name + "/comm";
        int fd = ::open(path.c_str(), O_RDONLY);
        if (fd < 0) continue;
        char buf[64]{};
        ssize_t n = ::read(fd, buf, sizeof(buf) - 1);
        ::close(fd);
        if (n > 0 && std::strncmp(buf, "cs2", 3) == 0) {
            found = true;
            break;
        }
    }
    closedir(d);
    return found;
}

// ---------- real input devices (Mouse4) ----------

struct InputDev {
    int fd = -1;
};

std::vector<InputDev> open_input_devices() {
    std::vector<InputDev> out;
    DIR* d = opendir("/dev/input");
    if (!d) return out;

    while (dirent* e = readdir(d)) {
        if (std::strncmp(e->d_name, "event", 5) != 0) continue;
        std::string path = std::string("/dev/input/") + e->d_name;
        int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;

        unsigned long evbits[64]{};
        if (ioctl(fd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0) {
            ::close(fd);
            continue;
        }
        const unsigned long mask = 1UL << (EV_KEY % (8 * sizeof(long)));
        const size_t idx = EV_KEY / (8 * sizeof(long));
        if (idx >= 64 || !(evbits[idx] & mask)) {
            ::close(fd);
            continue;
        }
        out.push_back({fd});
    }
    closedir(d);
    return out;
}

void close_input_devices(std::vector<InputDev>& devs) {
    for (auto& d : devs)
        if (d.fd >= 0) ::close(d.fd);
    devs.clear();
}

// Mouse4 = BTN_SIDE / BTN_EXTRA
bool is_mouse4(int code) {
    return code == BTN_SIDE || code == BTN_EXTRA || code == 0x113;
}

void poll_inputs(std::vector<InputDev>& devs) {
    input_event ev{};
    for (auto& d : devs) {
        while (true) {
            ssize_t n = ::read(d.fd, &ev, sizeof(ev));
            if (n != sizeof(ev)) break;
            if (ev.type != EV_KEY) continue;

            if (is_mouse4(ev.code) && ev.value == 1) {  // press
                g_bhop_on = !g_bhop_on;
                std::cout << (g_bhop_on ? "[BHOP ON]\n" : "[BHOP OFF]\n") << std::flush;
            }
        }
    }
}

}  // namespace

int main() {
    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    if (!open_uinput())
        return 1;

    auto devices = open_input_devices();
    if (devices.empty()) {
        std::cerr << "Warning: no readable /dev/input/event* (Mouse4 toggle may not work).\n"
                  << "         Try running as root.\n";
    }

    std::cout << "Standalone CS2 bhop ready.\n"
              << "  Bind:  Mouse Wheel Up → +jump\n"
              << "  Toggle: Mouse4 (side button)\n"
              << "  Only runs while CS2 process is alive\n"
              << "  Works also when standing still / AFK hop\n"
              << "  Ctrl+C to quit\n"
              << std::flush;

    auto last_wheel = clock::now();
    auto last_cs2_check = clock::now() - std::chrono::seconds(2);
    bool cs2_ok = false;

    while (g_running) {
        poll_inputs(devices);

        auto now = clock::now();
        if (now - last_cs2_check >= std::chrono::milliseconds(1500)) {
            bool was = cs2_ok;
            cs2_ok = cs2_running();
            last_cs2_check = now;
            if (was && !cs2_ok) {
                g_bhop_on = false;
                std::cout << "[CS2 gone – bhop forced OFF]\n" << std::flush;
            }
        }

        // no standing-still timeout → works in place / AFK
        if (g_bhop_on && cs2_ok) {
            if (now - last_wheel >= std::chrono::milliseconds(28)) {
                wheel_up();
                last_wheel = now;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    close_input_devices(devices);
    close_uinput();
    std::cout << "\nStopped.\n";
    return 0;
}
