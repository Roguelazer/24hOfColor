#include <pebble.h>

/* static global data */
static Window *s_main_window;
static Layer *s_face;
static Layer *s_hand;
static GFont s_time_font;
static GColor s_background_color;

/* function declarations */
static void main_window_load(Window*);
static void main_window_unload(Window*);
static void handle_tick(struct tm*, TimeUnits);
void update_window_color(struct tm*);
void trigger_redraw(void);
void handle_init(void);
void handle_deinit(void);

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define RADIAL_INSET 8

static void handle_tick(struct tm* tick_time, TimeUnits tick_changed)
{
	layer_mark_dirty(s_hand);
}

void trigger_redraw(void) {
	layer_mark_dirty(s_face);
	layer_mark_dirty(s_hand);
}


GColor companion_color(GColor x) {
	if (gcolor_equal(x, GColorWhite)) {
		return GColorLightGray;
	} else if (gcolor_equal(x, GColorBlack)) {
		return GColorDarkGray;
	} else {
		return GColorBlack;
	}
}

void update_window_color(struct tm* now)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "setting up window color");
	GColor colors[] = {
		GColorDarkGray,
		GColorLightGray,
		GColorBulgarianRose,
		GColorDarkCandyAppleRed,
		GColorOxfordBlue,
		GColorImperialPurple,
		GColorWindsorTan,
		GColorSunsetOrange,
		GColorVividViolet,
		GColorVividCerulean,
		GColorMediumAquamarine,
		GColorMelon,
		GColorOrange,
		GColorChromeYellow,
		GColorRajah,
	};

	s_background_color = COLOR_FALLBACK(colors[rand() % sizeof(colors)], GColorWhite);
	window_set_background_color(s_main_window, s_background_color);
}

void draw_face_layer(Layer* l, GContext* ctx)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing face layer");
	GColor foreground_color = gcolor_legible_over(s_background_color);
	GRect bounds = layer_get_frame(l);
	GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);
	int height = bounds.size.h - 2;
	int width = bounds.size.w - 2;

	graphics_context_set_stroke_color(ctx, foreground_color);
	graphics_context_set_fill_color(ctx, foreground_color);
	graphics_context_set_text_color(ctx, foreground_color);

	graphics_draw_circle(ctx, center, 6);
	graphics_fill_circle(ctx, center, 5);

	// draw 4 ticks
	GRect zero_box;
	zero_box.size.w = 30;
	zero_box.size.h = 12;
	zero_box.origin.x = width / 2 - 8;
	zero_box.origin.y = height - 20;
	graphics_draw_text(ctx, "0", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	zero_box.origin.y = height / 2 - 8;
	zero_box.origin.x = 2;
	graphics_draw_text(ctx, "6", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	zero_box.origin.x = width / 2 - 8;
	zero_box.origin.y = 0;
	graphics_draw_text(ctx, "12", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	zero_box.origin.y = height / 2 - 8;
	zero_box.origin.x = width - 30;
	graphics_draw_text(ctx, "18", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentRight, NULL);

	graphics_context_set_text_color(ctx, companion_color(foreground_color));

	int circle_radius = MIN(width / 2 - 2, height / 2 - 2);
	for (int i = 0; i < 24; ++i) {
		if (i % 6 == 0) {
			continue;
		}
		int angle = i * TRIG_MAX_ANGLE / 24;

		int y = (-cos_lookup(angle) * circle_radius / TRIG_MAX_RATIO);
		int x = (sin_lookup(angle) * circle_radius / TRIG_MAX_RATIO);

		GPoint pt = GPoint(x + center.x, y + center.y);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing point at for i=%d {%d, %d} (originally {%d, %d}, screen {%d, %d})", i, pt.x, pt.y, x, y, width, height);
		graphics_draw_circle(ctx, pt, 2);
		graphics_fill_circle(ctx, pt, 2);
	}
}

void draw_hand_layer(Layer* l, GContext* ctx)
{
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Drawing hand layer");
	GColor foreground_color = gcolor_legible_over(s_background_color);
	GRect bounds = layer_get_frame(l);
	GPoint center = GPoint(bounds.size.w / 2, bounds.size.h / 2);

	graphics_context_set_antialiased(ctx, true);

	graphics_context_set_stroke_color(ctx, foreground_color);
	graphics_context_set_fill_color(ctx, foreground_color);
	graphics_context_set_stroke_width(ctx, 3);

	time_t ts = time(NULL);
	struct tm* now = localtime(&ts);

	GRect reduced_bounds = bounds;
	reduced_bounds.origin.x += 18;
	reduced_bounds.origin.y += 18;
	reduced_bounds.size.w -= 36;
	reduced_bounds.size.h -= 36;

	// initially, angle is relative to top (wrong)
	int angle = TRIG_MAX_ANGLE * (now->tm_hour * 60 + now->tm_min) / 1440;
	// make 0 at the bottom
	angle = (angle + TRIG_MAX_ANGLE / 2) % TRIG_MAX_ANGLE;

	APP_LOG(APP_LOG_LEVEL_DEBUG, "angle=%d, TRIG_MAX_ANGLE/2=%d", angle, TRIG_MAX_ANGLE / 2);

	graphics_context_set_fill_color(ctx, companion_color(foreground_color));

	if (angle < TRIG_MAX_ANGLE / 2) {
		// first draw the bottom half
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2, TRIG_MAX_ANGLE);
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, RADIAL_INSET, TRIG_MAX_ANGLE / 2, TRIG_MAX_ANGLE);
		// then the rest
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, 0, angle);
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, RADIAL_INSET, 0, angle);
	} else {
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2, angle);
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, RADIAL_INSET, TRIG_MAX_ANGLE / 2, angle);
	}

	graphics_context_set_stroke_width(ctx, 2);

	int hand_length = MIN(reduced_bounds.size.h, reduced_bounds.size.w) / 2;

	GPoint hand_end;
	hand_end.y = (-cos_lookup(angle) * hand_length / TRIG_MAX_RATIO) + center.y;
	hand_end.x = (sin_lookup(angle) * hand_length / TRIG_MAX_RATIO) + center.x;

	graphics_draw_line(ctx, center, hand_end);
}

static void main_window_load(Window *window)
{
	/* initialize UI */
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_frame(window_layer);

	s_time_font = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);

	s_face = layer_create(bounds);
	layer_set_update_proc(s_face, draw_face_layer);

	time_t ts = time(NULL);
	struct tm* now = localtime(&ts);

	update_window_color(now);

	s_hand = layer_create(bounds);
	layer_set_update_proc(s_hand, draw_hand_layer);

	/* initialize event handlers */
	handle_tick(now, MINUTE_UNIT);
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);

	layer_add_child(window_layer, s_face);
	layer_add_child(window_layer, s_hand);
	trigger_redraw();
}

static void main_window_unload(Window *window)
{
	tick_timer_service_unsubscribe();
	layer_destroy(s_face);
}

void handle_init(void)
{
	s_main_window = window_create();

	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload,
	});
	window_stack_push(s_main_window, true);
}

void handle_deinit(void)
{
	app_message_deregister_callbacks();
	window_destroy(s_main_window);
}

int main(void)
{
	handle_init();
	app_event_loop();
	handle_deinit();
}

/* vim: set ts=4 sw=4 sts=0 noexpandtab: */
