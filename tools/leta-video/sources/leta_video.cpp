#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

static const uint16_t LETA_USB_VID = 0x0309;
static const uint16_t LETA_USB_VIDEO_PID = 0x2002;
static const int OLED_W = 128;
static const int OLED_H = 64;
static const int FRAME_SIZE = OLED_W * OLED_H / 8;
static const int REPORT_SIZE = 64;

static volatile sig_atomic_t keep_running = 1;

struct CaptureRegion {
	int x;
	int y;
	int w;
	int h;
};

struct Options {
	int fps = 15;
	int threshold = 128;
	bool invert = false;
	bool list = false;
	bool zero_report_id = true;
	std::string device;
};

static void on_signal(int)
{
	keep_running = 0;
}

static void print_usage(const char *program)
{
	printf("Usage: %s [options]\n", program);
	printf("Options:\n");
	printf("  --list                 list matching hidraw devices\n");
	printf("  --device /dev/hidrawN  use a specific hidraw device\n");
	printf("  --fps N                stream frame rate, default 15\n");
	printf("  --threshold N          luminance threshold 0..255, default 128\n");
	printf("  --invert               invert monochrome output\n");
	printf("  --zero-report-id       prefix each hidraw write with report ID 0, default\n");
	printf("  --raw-report           write 64-byte reports without a leading report ID\n");
	printf("  --help                 show this help\n");
}

static bool parse_int(const char *text, int *value)
{
	char *end = NULL;
	long parsed = strtol(text, &end, 0);
	if (end == text || *end != '\0') {
		return false;
	}

	*value = (int)parsed;
	return true;
}

static bool parse_options(int argc, char **argv, Options *options)
{
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];

		if (arg == "--help") {
			print_usage(argv[0]);
			exit(0);
		} else if (arg == "--list") {
			options->list = true;
		} else if (arg == "--invert") {
			options->invert = true;
		} else if (arg == "--zero-report-id") {
			options->zero_report_id = true;
		} else if (arg == "--raw-report") {
			options->zero_report_id = false;
		} else if (arg == "--device" && (i + 1) < argc) {
			options->device = argv[++i];
		} else if (arg == "--fps" && (i + 1) < argc) {
			if (!parse_int(argv[++i], &options->fps) || options->fps <= 0 || options->fps > 60) {
				fprintf(stderr, "Invalid --fps value\n");
				return false;
			}
		} else if (arg == "--threshold" && (i + 1) < argc) {
			if (!parse_int(argv[++i], &options->threshold) ||
					options->threshold < 0 || options->threshold > 255) {
				fprintf(stderr, "Invalid --threshold value\n");
				return false;
			}
		} else {
			fprintf(stderr, "Unknown or incomplete option: %s\n", arg.c_str());
			return false;
		}
	}

	return true;
}

static bool read_text_file(const std::string &path, std::string *out)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0) {
		return false;
	}

	char buffer[1024];
	ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
	close(fd);
	if (n <= 0) {
		return false;
	}

	buffer[n] = '\0';
	*out = buffer;
	return true;
}

static bool hidraw_matches_vid_pid(const std::string &hidraw_name, uint16_t vid, uint16_t pid)
{
	std::string uevent_path = "/sys/class/hidraw/" + hidraw_name + "/device/uevent";
	std::string uevent;
	if (!read_text_file(uevent_path, &uevent)) {
		return false;
	}

	char needle[32];
	snprintf(needle, sizeof(needle), ":%08X:%08X", vid, pid);
	if (uevent.find(needle) != std::string::npos) {
		return true;
	}

	snprintf(needle, sizeof(needle), ":%04X:%04X", vid, pid);
	return uevent.find(needle) != std::string::npos;
}

static std::vector<std::string> find_hidraw_devices()
{
	std::vector<std::string> devices;
	DIR *dir = opendir("/sys/class/hidraw");
	if (dir == NULL) {
		return devices;
	}

	while (dirent *entry = readdir(dir)) {
		if (strncmp(entry->d_name, "hidraw", 6) != 0) {
			continue;
		}

		if (hidraw_matches_vid_pid(entry->d_name, LETA_USB_VID, LETA_USB_VIDEO_PID)) {
			devices.push_back(std::string("/dev/") + entry->d_name);
		}
	}

	closedir(dir);
	std::sort(devices.begin(), devices.end());
	return devices;
}

static int open_video_device(const Options &options)
{
	std::string path = options.device;
	if (path.empty()) {
		std::vector<std::string> devices = find_hidraw_devices();
		if (devices.empty()) {
			fprintf(stderr, "No Leta USB Video HID device found (%04x:%04x)\n",
					LETA_USB_VID, LETA_USB_VIDEO_PID);
			return -1;
		}
		path = devices[0];
	}

	int fd = open(path.c_str(), O_WRONLY);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", path.c_str(), strerror(errno));
		return -1;
	}

	printf("Streaming to %s\n", path.c_str());
	return fd;
}

static void draw_rect(Display *display, Window root, GC gc, const CaptureRegion &r)
{
	if (r.w > 0 && r.h > 0) {
		XDrawRectangle(display, root, gc, r.x, r.y, (unsigned int)r.w, (unsigned int)r.h);
		XFlush(display);
	}
}

static CaptureRegion normalize_region(int x0, int y0, int x1, int y1)
{
	CaptureRegion r;
	r.x = std::min(x0, x1);
	r.y = std::min(y0, y1);
	r.w = std::abs(x1 - x0);
	r.h = std::abs(y1 - y0);
	return r;
}

static bool select_region(Display *display, Window root, CaptureRegion *selected)
{
	Cursor cursor = XCreateFontCursor(display, XC_crosshair);
	int grab = XGrabPointer(display, root, False,
			ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
			GrabModeAsync, GrabModeAsync, root, cursor, CurrentTime);
	if (grab != GrabSuccess) {
		fprintf(stderr, "Cannot grab pointer for selection\n");
		XFreeCursor(display, cursor);
		return false;
	}

	GC gc = XCreateGC(display, root, 0, NULL);
	XSetFunction(display, gc, GXxor);
	XSetForeground(display, gc, WhitePixel(display, DefaultScreen(display)));
	XSetSubwindowMode(display, gc, IncludeInferiors);
	XSetLineAttributes(display, gc, 1, LineSolid, CapButt, JoinMiter);

	bool pressed = false;
	bool drew = false;
	int x0 = 0;
	int y0 = 0;
	CaptureRegion last = {0, 0, 0, 0};

	printf("Drag-select the source region...\n");
	while (keep_running) {
		XEvent event;
		XNextEvent(display, &event);

		if (event.type == ButtonPress) {
			pressed = true;
			x0 = event.xbutton.x_root;
			y0 = event.xbutton.y_root;
			last = {x0, y0, 0, 0};
		} else if (event.type == MotionNotify && pressed) {
			while (XCheckTypedEvent(display, MotionNotify, &event)) {
			}

			if (drew) {
				draw_rect(display, root, gc, last);
			}
			last = normalize_region(x0, y0, event.xmotion.x_root, event.xmotion.y_root);
			draw_rect(display, root, gc, last);
			drew = true;
		} else if (event.type == ButtonRelease && pressed) {
			if (drew) {
				draw_rect(display, root, gc, last);
			}
			*selected = normalize_region(x0, y0, event.xbutton.x_root, event.xbutton.y_root);
			break;
		}
	}

	XUngrabPointer(display, CurrentTime);
	XFreeGC(display, gc);
	XFreeCursor(display, cursor);

	return selected->w > 0 && selected->h > 0;
}

static int mask_shift(unsigned long mask)
{
	int shift = 0;
	while (mask != 0 && (mask & 1UL) == 0) {
		mask >>= 1;
		shift++;
	}
	return shift;
}

static int mask_bits(unsigned long mask)
{
	int bits = 0;
	while (mask != 0) {
		if (mask & 1UL) {
			bits++;
		}
		mask >>= 1;
	}
	return bits;
}

static uint8_t scale_component(unsigned long pixel, unsigned long mask, int shift, int bits)
{
	if (mask == 0 || bits == 0) {
		return 0;
	}

	unsigned long value = (pixel & mask) >> shift;
	unsigned long max_value = (1UL << bits) - 1UL;
	return (uint8_t)((value * 255UL) / max_value);
}

static bool capture_frame(Display *display, Window root, Visual *visual,
		const CaptureRegion &region, const Options &options, uint8_t frame[FRAME_SIZE])
{
	XImage *image = XGetImage(display, root, region.x, region.y,
			(unsigned int)region.w, (unsigned int)region.h, AllPlanes, ZPixmap);
	if (image == NULL) {
		return false;
	}

	memset(frame, 0, FRAME_SIZE);

	int r_shift = mask_shift(visual->red_mask);
	int g_shift = mask_shift(visual->green_mask);
	int b_shift = mask_shift(visual->blue_mask);
	int r_bits = mask_bits(visual->red_mask);
	int g_bits = mask_bits(visual->green_mask);
	int b_bits = mask_bits(visual->blue_mask);

	for (int y = 0; y < OLED_H; y++) {
		int sy = (y * region.h) / OLED_H;
		if (sy >= region.h) {
			sy = region.h - 1;
		}

		for (int x = 0; x < OLED_W; x++) {
			int sx = (x * region.w) / OLED_W;
			if (sx >= region.w) {
				sx = region.w - 1;
			}

			unsigned long pixel = XGetPixel(image, sx, sy);
			uint8_t r = scale_component(pixel, visual->red_mask, r_shift, r_bits);
			uint8_t g = scale_component(pixel, visual->green_mask, g_shift, g_bits);
			uint8_t b = scale_component(pixel, visual->blue_mask, b_shift, b_bits);
			uint16_t lum = (uint16_t)((77U * r + 150U * g + 29U * b) >> 8);
			bool on = lum >= (uint16_t)options.threshold;
			if (options.invert) {
				on = !on;
			}

			if (on) {
				frame[(y / 8) * OLED_W + x] |= (uint8_t)(1U << (y & 7));
			}
		}
	}

	XDestroyImage(image);
	return true;
}

static bool write_all(int fd, const uint8_t *data, size_t length)
{
	size_t done = 0;
	while (done < length) {
		ssize_t n = write(fd, data + done, length - done);
		if (n < 0) {
			if (errno == EINTR) {
				continue;
			}
			return false;
		}
		done += (size_t)n;
	}
	return true;
}

static bool send_frame(int fd, const Options &options, const uint8_t frame[FRAME_SIZE])
{
	for (int offset = 0; offset < FRAME_SIZE; offset += REPORT_SIZE) {
		if (options.zero_report_id) {
			uint8_t report[REPORT_SIZE + 1];
			report[0] = 0;
			memcpy(&report[1], &frame[offset], REPORT_SIZE);
			if (!write_all(fd, report, sizeof(report))) {
				return false;
			}
		} else if (!write_all(fd, &frame[offset], REPORT_SIZE)) {
			return false;
		}
	}

	return true;
}

int main(int argc, char **argv)
{
	Options options;
	if (!parse_options(argc, argv, &options)) {
		print_usage(argv[0]);
		return 1;
	}

	if (options.list) {
		std::vector<std::string> devices = find_hidraw_devices();
		for (size_t i = 0; i < devices.size(); i++) {
			printf("%s\n", devices[i].c_str());
		}
		return devices.empty() ? 1 : 0;
	}

	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);

	int fd = open_video_device(options);
	if (fd < 0) {
		return 2;
	}

	Display *display = XOpenDisplay(NULL);
	if (display == NULL) {
		fprintf(stderr, "Cannot open X display\n");
		close(fd);
		return 3;
	}

	int screen = DefaultScreen(display);
	Window root = RootWindow(display, screen);
	Visual *visual = DefaultVisual(display, screen);

	CaptureRegion region = {0, 0, 0, 0};
	if (!select_region(display, root, &region)) {
		fprintf(stderr, "No valid region selected\n");
		XCloseDisplay(display);
		close(fd);
		return 4;
	}

	printf("Selected %dx%d at %+d%+d, streaming %d fps\n",
			region.w, region.h, region.x, region.y, options.fps);

	uint8_t frame[FRAME_SIZE];
	useconds_t frame_delay_us = (useconds_t)(1000000 / options.fps);

	while (keep_running) {
		if (!capture_frame(display, root, visual, region, options, frame)) {
			fprintf(stderr, "Capture failed\n");
			break;
		}

		if (!send_frame(fd, options, frame)) {
			fprintf(stderr, "USB write failed: %s\n", strerror(errno));
			break;
		}

		usleep(frame_delay_us);
	}

	XCloseDisplay(display);
	close(fd);
	printf("Stopped\n");
	return 0;
}
