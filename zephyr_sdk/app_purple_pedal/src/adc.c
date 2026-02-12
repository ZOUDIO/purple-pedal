
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include "common.h"
// NickR: Include curve
#include "curve.h"

LOG_MODULE_REGISTER(app_adc, CONFIG_APP_LOG_LEVEL);

// #define ADC_NODE_ID DT_ALIAS(pedal_adc)
// #define ADC_CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE_ID)
// #define ADC_SAMPLE_PERIOD K_MSEC(10)
// #define ADC_SAMPLE_PERIOD K_USEC(1000) // NickR: Already in common.h

// #define CONFIG_SEQUENCE_SAMPLES (1)
// #define ADC_NUM_BITS (12)
#define ADC_VAL_MIN (0)
#define ADC_VAL_MAX (BIT_MASK(ADC_NUM_BITS))
#define ADC_VAL_MID (BIT(ADC_NUM_BITS-1))

#define ADC_GAIN (128)
#define LOAD_CELL_MV_V (1)
#define LOAD_CELL_MAX ((uint64_t)ADC_VAL_MID * ADC_GAIN * LOAD_CELL_MV_V / 1000ULL)

#define APP_ADC_STACK_SIZE 4096
#define APP_ADC_PRIORITY 5

// Storage for the previous value of each channel (EMA filtering)
static int32_t ema_state[3] = {0, 0, 0};

// --- HELPER FUNCTION ---
int32_t apply_ema_filter(int ch_idx, int32_t raw_new) {
    // Standard EMA Formula: Output = Alpha * New + (1-Alpha) * Old
    // We use bit shifting (>>8 is /256) to avoid slow floating point math
    int64_t filtered = ((int64_t)FILTER_ALPHA * raw_new 
                      + (int64_t)(256 - FILTER_ALPHA) * ema_state[ch_idx]);
    
    // Update state and return
    ema_state[ch_idx] = (int32_t)(filtered >> 8); 
    return ema_state[ch_idx];
}

ZBUS_CHAN_DECLARE(gamepad_report_out_chan);
ZBUS_CHAN_DECLARE(gamepad_feature_report_raw_val_chan);

struct app_adc_ctx{
	struct k_work_q *workq;
	struct k_work work;
	const struct device *adc;
	const struct adc_channel_cfg ch_cfgs[ADC_CHANNEL_COUNT];
	const struct adc_sequence_options adc_seq_opt;
	struct adc_sequence seq;
	// const struct gamepad_calibration *calibration; // We now take the global global current_calib value
	int32_t channel_reading[CONFIG_SEQUENCE_SAMPLES][ADC_CHANNEL_COUNT];
} ;

// enum adc_action adc_seq_cb(const struct device *dev,const struct adc_sequence *sequence,uint16_t sampling_index)
// {
// 	uint16_t (*reading)[CHANNEL_COUNT] = (uint16_t (*)[CHANNEL_COUNT])sequence->buffer;
// 	struct gamepad_report_out rpt = {
// 		.accelerator = (int32_t)reading[0][0] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
// 		.brake = (int32_t)reading[0][1] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
// 		.clutch = (int32_t)reading[0][2] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
// 	};
// 	int err = zbus_chan_pub(&gamepad_report_out_chan, &rpt, K_MSEC(2));

// 	struct app_adc_ctx *ctx = (struct app_adc_ctx *)sequence->options->user_data;
// 	err = k_sem_take(&ctx->sem_stop, K_NO_WAIT);
// 	if(err == 0)
// 		return ADC_ACTION_FINISH;
// 	else
// 		return ADC_ACTION_REPEAT;
// }

//uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];

//forward declaration
static void app_adc_work_handler(struct k_work *work);

K_THREAD_STACK_DEFINE(app_adc_workq_stack_area, APP_ADC_STACK_SIZE);
static struct k_work_q app_adc_workq;
static struct app_adc_ctx ctx = {
	.workq = &app_adc_workq,
	.work = Z_WORK_INITIALIZER(app_adc_work_handler),
	.adc = DEVICE_DT_GET(ADC_NODE_ID),
	.ch_cfgs={DT_FOREACH_CHILD_SEP(ADC_NODE_ID, ADC_CHANNEL_CFG_DT, (,))},
	.adc_seq_opt = {
		.extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
		.interval_us = 0,
		//.callback = adc_seq_cb,
		.user_data = &ctx,
	},
	.seq = {
		.channels = BIT_MASK(ADC_CHANNEL_COUNT),
		.buffer = ctx.channel_reading,
		.buffer_size = 	SIZEOF_FIELD(struct app_adc_ctx, channel_reading),
		//.resolution = 12,
		.resolution = 24,
		//.resolution = DT_PROP(ADC_NODE_ID, resolution),
		.options = &ctx.adc_seq_opt,
	},
	//.sem_stop = Z_SEM_INITIALIZER(ctx.sem_stop, 0, 1),
};

// static void app_adc_work_handler(struct k_work *work)
// {
// 	struct app_adc_ctx *ctx = CONTAINER_OF(work, struct app_adc_ctx, work);
// 	int err;
// 	if (!device_is_ready(ctx->adc)) {
// 		LOG_ERR("ADC controller device %s not ready\n", ctx->adc->name);
// 	}
// 	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
// 		err = adc_channel_setup(ctx->adc, &ctx->ch_cfgs[i]);
// 		if (err < 0) {
// 			LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
// 		}
// 	}
// 	err = adc_read(ctx->adc, &ctx->seq);
// 	if (err < 0){
// 		LOG_ERR("adc_read() error (%d)\n", err);
// 	}
// 	LOG_DBG("app_adc_work_handler() ended");
// }

inline uint16_t raw_to_uint16(int32_t raw, int32_t offset, int32_t scale)
{
	//clamp the <offset values. also when loadcell is disconnected, we should output 0
	if(raw < offset || raw <= LOAD_CELL_DISCONNECT_THRESHOLD) return 0;

	int64_t val_64 = ((int64_t)(raw - offset)) * UINT16_MAX / scale;
	return (val_64 > UINT16_MAX)? UINT16_MAX : (uint16_t)val_64;
}


static void app_adc_work_handler(struct k_work *work)
{
	struct app_adc_ctx *ctx = CONTAINER_OF(work, struct app_adc_ctx, work);
	//int64_t begin = k_uptime_get();
	int err = adc_read(ctx->adc, &ctx->seq);
	//LOG_DBG("adc time: %d ms", (int32_t)(k_uptime_get()-begin));
	if (err < 0){
		LOG_ERR("adc_read() error (%d)\n", err);
	}
	
	// NickR: Extract Raw Values
	int32_t raw_clu = ctx->channel_reading[0][SETTING_INDEX_CLUTCH];
    int32_t raw_brk = ctx->channel_reading[0][SETTING_INDEX_BRAKE];
    int32_t raw_acc = ctx->channel_reading[0][SETTING_INDEX_ACCELERATOR];

	// UPDATE CONNECTION FLAGS (Critical for LED Logic)
    // Logic: If Raw Value is small, it's disconnected.
    // We update the global 'pedal_connected_flags' bits:
    // Bit 0 = Clutch, Bit 1 = Brake, Bit 2 = Accelerator
    uint8_t flags = 0;

    if (raw_clu > LOAD_CELL_DISCONNECT_THRESHOLD) flags |= (1 << SETTING_INDEX_CLUTCH);
    if (raw_brk > LOAD_CELL_DISCONNECT_THRESHOLD) flags |= (1 << SETTING_INDEX_BRAKE);
    if (raw_acc > LOAD_CELL_DISCONNECT_THRESHOLD) flags |= (1 << SETTING_INDEX_ACCELERATOR);

    pedal_connected_flags = flags; // Write to Global

    // NickR: Apply EMA Filter
    int32_t filt_clu = apply_ema_filter(SETTING_INDEX_CLUTCH, raw_clu);
    int32_t filt_brk = apply_ema_filter(SETTING_INDEX_BRAKE, raw_brk);
    int32_t filt_acc = apply_ema_filter(SETTING_INDEX_ACCELERATOR, raw_acc);

	// NickR: Linear output
	uint16_t linear_clu = raw_to_uint16(filt_clu,
					current_calib.offset[SETTING_INDEX_CLUTCH], 
					current_calib.scale[SETTING_INDEX_CLUTCH]);
	uint16_t linear_brk = raw_to_uint16(filt_brk,
					current_calib.offset[SETTING_INDEX_BRAKE], 
					current_calib.scale[SETTING_INDEX_BRAKE]);
	uint16_t linear_acc = raw_to_uint16(filt_acc,
                    current_calib.offset[SETTING_INDEX_ACCELERATOR], 
                    current_calib.scale[SETTING_INDEX_ACCELERATOR]);
	// NickR: Curve transformation (if set)
	uint16_t final_clu = curve_apply(SETTING_INDEX_CLUTCH, linear_clu);
	uint16_t final_brk = curve_apply(SETTING_INDEX_BRAKE, linear_brk);
    uint16_t final_acc = curve_apply(SETTING_INDEX_ACCELERATOR, linear_acc);

	// NickR: Changed to EMA filtered values
	struct gamepad_feature_rpt_raw_val rpt_raw_val = {
		.report_id = GAMEPAD_FEATURE_REPORT_RAW_VAL_ID,
		.clutch_raw = filt_clu,
		.brake_raw = filt_brk,
		.accelerator_raw = filt_acc,
		// No EMA filtering
		//.accelerator_raw = ctx->channel_reading[0][SETTING_INDEX_ACCELERATOR],
		//.brake_raw = ctx->channel_reading[0][SETTING_INDEX_BRAKE],
		//.clutch_raw = ctx->channel_reading[0][SETTING_INDEX_CLUTCH],
	};

	// NickR: Changed to EMA filtered values
	struct gamepad_report_out rpt = {
		.report_id = GAMEPAD_INPUT_REPORT_ID,
		.clutch = final_clu,
		.brake = final_brk,
		.accelerator = final_acc,
	};
	//LOG_INF("gamepad report: accelerator = %d, brake = %d, clutch = %d",rpt.accelerator, rpt.brake, rpt.clutch);

	// struct gamepad_report_out rpt = {
	// 	.accelerator = 100,
	// 	.brake = 200,
	// 	.clutch = 300,
	// };
	//int64_t acc = (int64_t)ctx->channel_reading[0][0] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN);
	//LOG_ERR("acc(%lld)\n", (ADC_VAL_MAX - ADC_VAL_MIN));
	
	// NickR: Changed to K_NO_WAIT
	err = zbus_chan_pub(&gamepad_report_out_chan, &rpt, K_NO_WAIT); //ADC_SAMPLE_PERIOD);
	if (err < 0){
		LOG_ERR("zbus_chan_pub(&gamepad_report_out_chan) error (%d)\n", err);
	}
	err = zbus_chan_pub(&gamepad_feature_report_raw_val_chan, &rpt_raw_val, K_NO_WAIT); //ADC_SAMPLE_PERIOD);
	if (err < 0){
		LOG_ERR("zbus_chan_pub(&gamepad_feature_report_raw_val_chan) error (%d)\n", err);
	}				
	//LOG_DBG("app_adc_work_handler() ended");
}

static void app_adc_timer_handler(struct k_timer *timer)
{
	struct app_adc_ctx *ctx = (struct app_adc_ctx *)k_timer_user_data_get(timer);
	//check if the work is already busy, avoid resubmitting
	if (k_work_busy_get(&ctx->work)){
		LOG_INF("adc already working.");
	}
	else{
		int err = k_work_submit_to_queue(ctx->workq, &ctx->work);
		if (err < 0){
			LOG_ERR("k_work_submit_to_queue() returns %d", err);
		}
	}
}
static K_TIMER_DEFINE(app_adc_timer, app_adc_timer_handler, NULL);

// static void gamepad_adc_ctrl_cb(const struct zbus_channel *chan)
// {
// 	int err;
// 	const enum app_adc_action *action = zbus_chan_const_msg(chan);

// 	//TODO: perform start / stop actions
// 	if(*action == APP_ADC_ACTION_START){
// 		//check if the work is already busy, avoid resubmitting
// 		if(k_work_busy_get(&ctx.work)){
// 			LOG_INF("adc already working.");
// 		}
// 		else{
// 			err = k_work_submit_to_queue(&app_adc_work_q, &ctx.work);
// 			if(err<0){
// 				LOG_ERR("k_work_submit_to_queue() returns %d", err);
// 			}
// 		}
// 		return;
// 	}

// 	if(*action == APP_ADC_ACTION_STOP){
// 		k_sem_give(&ctx.sem_stop);
// 		return;
// 	}
// }

// static void gamepad_adc_ctrl_cb(const struct zbus_channel *chan)
// {
// 	const enum app_adc_action *action = zbus_chan_const_msg(chan);
// 	//perform start / stop actions
// 	if(*action == APP_ADC_ACTION_START){
// 		k_timer_start(&app_adc_timer, K_MSEC(0), ADC_SAMPLE_PERIOD);
// 		return;
// 	}

// 	if(*action == APP_ADC_ACTION_STOP){
// 		k_timer_stop(&app_adc_timer);
// 		return;
// 	}
// }
// ZBUS_LISTENER_DEFINE(gp_adc_ctrl_handler, gamepad_adc_ctrl_cb);

static void gamepad_adc_ctrl_cb(const struct zbus_channel *chan)
{
	const enum app_state *state = zbus_chan_const_msg(chan);
	if(*state == APP_STATE_HID_WORKING){
		LOG_DBG("start ADC timer");
		k_timer_start(&app_adc_timer, K_MSEC(0), ADC_SAMPLE_PERIOD);
		return;
	}
	else{
		LOG_DBG("stop ADC timer");
		k_timer_stop(&app_adc_timer);
		return;
	}
}
ZBUS_LISTENER_DEFINE(gp_adc_ctrl_handler, gamepad_adc_ctrl_cb);

int app_adc_init(void)
{
	int err;
	if (!device_is_ready(ctx.adc)){
		LOG_ERR("ADC controller device %s not ready\n", ctx.adc->name);
	}
	for (size_t i = 0U; i < ADC_CHANNEL_COUNT; i++){
		err = adc_channel_setup(ctx.adc, &ctx.ch_cfgs[i]);
		if (err < 0){
			LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
		}
	}
	k_timer_user_data_set(&app_adc_timer, &ctx);
	k_work_queue_init(&app_adc_workq);
	k_work_queue_start(&app_adc_workq, app_adc_workq_stack_area,K_THREAD_STACK_SIZEOF(app_adc_workq_stack_area), APP_ADC_PRIORITY,NULL);

	//get the pointer to calibration settings
	// ctx.calibration = get_calibration(); // We now get the calibration from global current_calib
	return 0;
}
