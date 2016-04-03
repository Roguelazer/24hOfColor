#include <pebble.h>

/* static global data */
static Window *s_main_window;
static Layer *s_face;
static Layer *s_hand;
static GFont s_time_font;
static GColor s_background_color;
static GColor s_complementary_color;
static Animation* s_my_animation_ptr = NULL;

#define UPDATE_COLOR_EVERY_N_SECONDS 300
static time_t last_color_change = 0;
static int inset_pct = 5;

/* function declarations */
static void main_window_load(Window*);
static void main_window_unload(Window*);
static void handle_tick(struct tm*, TimeUnits);
void update_window_color();
void handle_init(void);
void handle_deinit(void);
void draw_animation_frame(Animation* animation, const AnimationProgress progress);

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void draw_animation_frame(Animation *animation, const AnimationProgress progress) {
	inset_pct = (progress * 100) / ANIMATION_NORMALIZED_MAX;
	layer_mark_dirty(s_hand);
}

static void handle_tick(struct tm* tick_time, TimeUnits tick_changed)
{
	time_t now = time(NULL);
	if ((now - last_color_change) > UPDATE_COLOR_EVERY_N_SECONDS) {
		update_window_color();
		last_color_change = now;
		layer_mark_dirty(s_face);
	}
	layer_mark_dirty(s_hand);
}

void update_window_color()
{
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

	GColor complementary_colors[] = {
		GColorLightGray,
		GColorDarkGray,
		GColorSunsetOrange,
		GColorMelon,
		GColorPictonBlue,
		GColorLavenderIndigo,
		GColorBrass,
		GColorRajah,
		GColorRichBrilliantLavender,
		GColorBabyBlueEyes,
		GColorMintGreen,
		GColorPastelYellow,
		GColorChromeYellow,
		GColorOrange,
		GColorArmyGreen,
	};

	srand(0);
	int array_offset = rand() % sizeof(colors);
	s_background_color = COLOR_FALLBACK(colors[array_offset], GColorWhite);
	window_set_background_color(s_main_window, s_background_color);

	APP_LOG(APP_LOG_LEVEL_DEBUG, "setting window color to %d, %d, %d", s_background_color.r, s_background_color.g, s_background_color.b);

	s_complementary_color = COLOR_FALLBACK(complementary_colors[array_offset], GColorBlack);
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
	graphics_context_set_fill_color(ctx, s_complementary_color);
	graphics_context_set_text_color(ctx, foreground_color);

	graphics_draw_circle(ctx, center, 6);
	graphics_fill_circle(ctx, center, 5);

	// draw 4 ticks
	GRect zero_box;
	zero_box.size.w = 30;
	zero_box.size.h = 12;
	zero_box.origin.x = width / 2 - 12;
	zero_box.origin.y = height - 20;
	graphics_draw_text(ctx, "0", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	zero_box.origin.y = height / 2 - 12;
	zero_box.origin.x = 2;
	graphics_draw_text(ctx, "6", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	zero_box.origin.x = width / 2 - 10;
	zero_box.origin.y = 0;
	graphics_draw_text(ctx, "12", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
	zero_box.origin.y = height / 2 - 10;
	zero_box.origin.x = width - 30;
	graphics_draw_text(ctx, "18", s_time_font, zero_box, GTextOverflowModeFill, GTextAlignmentRight, NULL);

	graphics_context_set_fill_color(ctx, s_complementary_color);
	int circle_inset = PBL_IF_ROUND_ELSE(6, 2);
	int circle_radius = MIN(width / 2 - circle_inset, height / 2 - circle_inset);
	for (int i = 0; i < 24; ++i) {
		if (i % 6 == 0) {
			continue;
		}
		int angle = i * TRIG_MAX_ANGLE / 24;

		int y = (-cos_lookup(angle) * circle_radius / TRIG_MAX_RATIO);
		int x = (sin_lookup(angle) * circle_radius / TRIG_MAX_RATIO);

		GPoint pt = GPoint(x + center.x, y + center.y);
		graphics_fill_circle(ctx, pt, 2);
	}
}

void draw_hand_layer(Layer* l, GContext* ctx)
{
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
	int arc_offset = PBL_IF_ROUND_ELSE(24, 18);
	reduced_bounds.origin.x += arc_offset;
	reduced_bounds.origin.y += arc_offset;
	reduced_bounds.size.w -= (arc_offset << 1);
	reduced_bounds.size.h -= (arc_offset << 1);

	// initially, angle is relative to top (wrong)
	int angle = TRIG_MAX_ANGLE * (now->tm_hour * 60 + now->tm_min) / 1440;
	// make 0 at the bottom
	angle = (angle + TRIG_MAX_ANGLE / 2) % TRIG_MAX_ANGLE;

	graphics_context_set_fill_color(ctx, s_complementary_color);

	int hand_length = MIN(reduced_bounds.size.h, reduced_bounds.size.w) / 2;
	int radial_inset = ((hand_length - 20) * inset_pct) / 100;

	if (angle < TRIG_MAX_ANGLE / 2) {
		// first draw the bottom half
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, radial_inset, TRIG_MAX_ANGLE / 2, TRIG_MAX_ANGLE);
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2, TRIG_MAX_ANGLE);
		// then the rest
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, radial_inset, 0, angle);
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, 0, angle);
	} else {
		graphics_fill_radial(ctx, reduced_bounds, GOvalScaleModeFitCircle, radial_inset, TRIG_MAX_ANGLE / 2, angle);
		graphics_draw_arc(ctx, reduced_bounds, GOvalScaleModeFitCircle, TRIG_MAX_ANGLE / 2, angle);
	}

	graphics_context_set_stroke_width(ctx, 2);

	GPoint hand_end;
	hand_end.y = (-cos_lookup(angle) * hand_length / TRIG_MAX_RATIO) + center.y;
	hand_end.x = (sin_lookup(angle) * hand_length / TRIG_MAX_RATIO) + center.x;

	graphics_draw_line(ctx, center, hand_end);

	GPoint sm_end = GPoint(center.x, center.y + hand_length);
	GPoint sm_start = GPoint(center.x, sm_end.y - radial_inset);

	graphics_draw_line(ctx, sm_start, sm_end);
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

	update_window_color();

	s_hand = layer_create(bounds);
	layer_set_update_proc(s_hand, draw_hand_layer);

	layer_add_child(window_layer, s_face);
	layer_add_child(window_layer, s_hand);

	/* initial draw of the face */
	layer_mark_dirty(s_face);

	static const AnimationImplementation impl = {
		.update = draw_animation_frame
	};

	s_my_animation_ptr = animation_create();
	animation_set_implementation(s_my_animation_ptr, &impl);
	animation_set_duration(s_my_animation_ptr, 1000);
	animation_schedule(s_my_animation_ptr);

	/* initialize event handlers */
	handle_tick(now, MINUTE_UNIT);
	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
}

static void main_window_unload(Window *window)
{
	animation_unschedule_all();
	tick_timer_service_unsubscribe();
	layer_destroy(s_face);
	layer_destroy(s_hand);
	animation_destroy(s_my_animation_ptr);
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
