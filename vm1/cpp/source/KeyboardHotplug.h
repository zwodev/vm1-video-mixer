#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <libudev.h>
#include <poll.h>
#include <cstring>
#include <errno.h>

class KeyboardHotplug {
public:
    KeyboardHotplug() : udev(nullptr), mon(nullptr) {}

    ~KeyboardHotplug() {
        stop();
    }

    bool start() {
        udev = udev_new();
        if (!udev) {
            std::cerr << "Failed to create udev\n";
            return false;
        }

        mon = udev_monitor_new_from_netlink(udev, "udev");
        if (!mon) {
            std::cerr << "Failed to create udev monitor\n";
            udev_unref(udev);
            return false;
        }

        udev_monitor_filter_add_match_subsystem_devtype(mon, "input", NULL);
        udev_monitor_enable_receiving(mon);

        int mon_fd = udev_monitor_get_fd(mon);

        // Initial scan: open all existing keyboard devices
        scanExistingKeyboards();

        poll_fds.push_back({mon_fd, POLLIN, 0});

        return true;
    }

    void stop() {
        for (auto& kv : device_fds) {
            close(kv.first);
        }
        device_fds.clear();
        poll_fds.clear();

        if (mon) {
            udev_monitor_unref(mon);
            mon = nullptr;
        }
        if (udev) {
            udev_unref(udev);
            udev = nullptr;
        }
    }

    std::vector<input_event> getAllEvents()
    {
        std::vector<input_event> events;
        int ret = poll(poll_fds.data(), poll_fds.size(), 16);
        if (ret < 0) {
            perror("poll");
            return events;
        }
        if (ret == 0) {
            //std::cout << "poll timeout\n";
            return events;
        }

        // Check udev monitor fd first
        if (poll_fds[0].revents & POLLIN) {
            handleUdevEvent();
        }

        for (size_t i = 0; i < poll_fds.size(); ++i) {
            if (poll_fds[i].revents != 0) {
                input_event event;
                while (readInputEvent(poll_fds[i].fd, event)) {
                    events.push_back(event);
                }
            }
        }

        return events;
    }

private:
    struct udev* udev;
    struct udev_monitor* mon;

    std::vector<struct pollfd> poll_fds; // First is udev monitor fd, rest are keyboard device fds
    std::map<int, std::string> device_fds; // fd -> devnode path

    void scanExistingKeyboards() {
        struct udev_enumerate* enumerate = udev_enumerate_new(udev);
        udev_enumerate_add_match_subsystem(enumerate, "input");
        udev_enumerate_scan_devices(enumerate);

        struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
        struct udev_list_entry* entry;

        udev_list_entry_foreach(entry, devices) {
            const char* path = udev_list_entry_get_name(entry);
            struct udev_device* dev = udev_device_new_from_syspath(udev, path);
            if (isKeyboardDevice(dev)) {
                const char* devnode = udev_device_get_devnode(dev);
                if (devnode)
                    openKeyboardDevice(devnode);
            }
            udev_device_unref(dev);
        }

        udev_enumerate_unref(enumerate);
    }

    bool isKeyboardDevice(struct udev_device* dev) {
        if (!dev) return false;

        const char* devnode = udev_device_get_devnode(dev);
        if (!devnode) return false;

        // Check if device node name contains "event"
        std::string devnode_str(devnode);
        if (devnode_str.find("event") == std::string::npos) return false;

        // Check if device has ID_INPUT_KEYBOARD property
        const char* prop = udev_device_get_property_value(dev, "ID_INPUT_KEYBOARD");
        if (prop && std::string(prop) == "1") return true;

        return false;
    }

    void openKeyboardDevice(const char* devnode) {
        int fd = open(devnode, O_RDONLY | O_NONBLOCK);
        if (fd < 0) {
            std::cerr << "Failed to open keyboard device " << devnode << ": " << strerror(errno) << "\n";
            return;
        }
        device_fds[fd] = devnode;
        poll_fds.push_back({fd, POLLIN, 0});
        std::cout << "Opened keyboard: " << devnode << "\n";
    }

    void closeKeyboardDevice(int fd) {
        close(fd);
        std::cout << "Closed keyboard: " << device_fds[fd] << "\n";
        device_fds.erase(fd);
        // Remove from poll_fds
        for (auto it = poll_fds.begin(); it != poll_fds.end(); ++it) {
            if (it->fd == fd) {
                poll_fds.erase(it);
                break;
            }
        }
    }

    void handleUdevEvent() {
        struct udev_device* dev = udev_monitor_receive_device(mon);
        if (!dev) return;

        const char* action = udev_device_get_action(dev);
        const char* devnode = udev_device_get_devnode(dev);

        if (!action || !devnode) {
            udev_device_unref(dev);
            return;
        }

        std::string action_str(action);
        if (action_str == "add") {
            if (isKeyboardDevice(dev)) {
                openKeyboardDevice(devnode);
            }
        } else if (action_str == "remove") {
            // Close keyboard if device removed
            for (auto& kv : device_fds) {
                if (kv.second == devnode) {
                    closeKeyboardDevice(kv.first);
                    break;
                }
            }
        }
        udev_device_unref(dev);
    }

    bool readInputEvent(int fd, input_event& ev) {
        ssize_t n = read(fd, &ev, sizeof(ev));
        if (n == (ssize_t)sizeof(ev)) {
            return true;
        } else if (n == 0) {
            std::cerr << "EOF on fd " << fd << std::endl;
            closeKeyboardDevice(fd);
            return false;
        } else if (n == -1 && errno != EAGAIN) {
            std::cerr << "Error reading input event: " << strerror(errno) << std::endl;
            closeKeyboardDevice(fd);
            return false;
        }
        // For n == -1 with EAGAIN or partial reads:
        return false;
    }
};
