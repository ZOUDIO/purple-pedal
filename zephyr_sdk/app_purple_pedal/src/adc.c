
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(app_adc, CONFIG_APP_LOG_LEVEL);

#define ADC_NODE_ID DT_ALIAS(pedal_adc)
#define ADC_CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE_ID)
#define ADC_SAMPLE_PERIOD K_MSEC(10)

#define CONFIG_SEQUENCE_SAMPLES (1)
#define ADC_NUM_BITS (12)
#define ADC_VAL_MIN (0)
#define ADC_VAL_MAX (BIT_MASK(ADC_NUM_BITS))

#define APP_ADC_STACK_SIZE 4096
#define APP_ADC_PRIORITY 5

ZBUS_CHAN_DECLARE(gamepad_report_out_chan);

struct app_adc_ctx{
	struct k_work_q *workq;
	struct k_work work;
	const struct device *adc;
	const struct adc_channel_cfg ch_cfgs[DT_CHILD_NUM(ADC_NODE_ID)];
	const struct adc_sequence_options adc_seq_opt;
	struct adc_sequence seq;
	struct k_sem sem_stop;
	uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][ADC_CHANNEL_COUNT];
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
		.channels = BIT_MASK(DT_CHILD_NUM(ADC_NODE_ID)),
		.buffer = ctx.channel_reading,
		.buffer_size = 	SIZEOF_FIELD(struct app_adc_ctx, channel_reading),
		.resolution = 12,
		.options = &ctx.adc_seq_opt,
	},
	.sem_stop = Z_SEM_INITIALIZER(ctx.sem_stop, 0, 1),
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

static void app_adc_work_handler(struct k_work *work)
{
	struct app_adc_ctx *ctx = CONTAINER_OF(work, struct app_adc_ctx, work);
	int err = adc_read(ctx->adc, &ctx->seq);
	if (err < 0){
		LOG_ERR("adc_read() error (%d)\n", err);
	}
	struct gamepad_report_out rpt = {
		.accelerator = (int32_t)ctx->channel_reading[0][0] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
		.brake = (int32_t)ctx->channel_reading[0][1] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
		.clutch = (int32_t)ctx->channel_reading[0][2] * (GAMEPAD_REPORT_VALUE_MAX - GAMEPAD_REPORT_VALUE_MIN) / (ADC_VAL_MAX - ADC_VAL_MIN) + GAMEPAD_REPORT_VALUE_MIN,
	};
	err = zbus_chan_pub(&gamepad_report_out_chan, &rpt, K_MSEC(2));
	if (err < 0){
		LOG_ERR("zbus_chan_pub() error (%d)\n", err);
	}		
	LOG_DBG("app_adc_work_handler() ended");
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
static void gamepad_adc_ctrl_cb(const struct zbus_channel *chan)
{
	const enum app_adc_action *action = zbus_chan_const_msg(chan);
	//perform start / stop actions
	if(*action == APP_ADC_ACTION_START){
		k_timer_start(&app_adc_timer, K_MSEC(0), ADC_SAMPLE_PERIOD);
		return;
	}

	if(*action == APP_ADC_ACTION_STOP){
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
	return 0;
}
