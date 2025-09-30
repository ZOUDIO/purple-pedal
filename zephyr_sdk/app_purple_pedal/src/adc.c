
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include "common.h"

LOG_MODULE_REGISTER(app_adc, CONFIG_APP_LOG_LEVEL);

#define ADC_NODE DT_ALIAS(pedal_adc)
//static const struct device *adc = DEVICE_DT_GET(ADC_NODE);
// static const struct adc_channel_cfg channel_cfgs[] = {
// 	DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))};
#define CHANNEL_COUNT DT_CHILD_NUM(ADC_NODE)
#define CONFIG_SEQUENCE_SAMPLES 10

#define APP_ADC_STACK_SIZE 1024
#define APP_ADC_PRIORITY 5

//static K_SEM_DEFINE(app_adc_sem, 0, 1);
ZBUS_CHAN_DECLARE(gamepad_report_out_chan);

struct app_adc_ctx{
	struct k_work work;
	const struct device *adc;
	const struct adc_channel_cfg ch_cfgs[DT_CHILD_NUM(ADC_NODE)];
	const struct adc_sequence_options adc_seq_opt;
	struct adc_sequence seq;
	struct k_sem sem_stop;
	uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];
} ;

enum adc_action adc_seq_cb(const struct device *dev,const struct adc_sequence *sequence,uint16_t sampling_index)
{
	//here we can return ADC_ACTION_REPEAT to repeat the ADC until
	//return ADC_ACTION_FINISH to end the sequence adc_read() will exit.
	//how to detect app_adc_stop() message at here?
	//1. another semaphore
	//2. atomic values
	//3. workqueue
	struct gamepad_report_out rpt = {0};
	int err = zbus_chan_pub(&gamepad_report_out_chan, &rpt, K_MSEC(10));

	struct app_adc_ctx *ctx = (struct app_adc_ctx *)sequence->options->user_data;
	err = k_sem_take(&ctx->sem_stop, K_NO_WAIT);
	if(err == 0)
		return ADC_ACTION_FINISH;
	else
		return ADC_ACTION_REPEAT;
}

// const struct adc_sequence_options options = {
//     .extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
//     .interval_us = 10000,
// 	.callback = adc_seq_cb,
// };

uint16_t channel_reading[CONFIG_SEQUENCE_SAMPLES][CHANNEL_COUNT];

static void app_adc_work_handler(struct k_work *work);

// struct adc_sequence sequence = {
//     .buffer = channel_reading,
//     /* buffer size in bytes, not number of samples */
//     .buffer_size = sizeof(channel_reading),
//     .resolution = 16,
//     .options = &options,
// };



K_THREAD_STACK_DEFINE(app_adc_workq_stack_area, APP_ADC_STACK_SIZE);
static struct k_work_q app_adc_work_q;
static struct app_adc_ctx ctx = {
	.work = Z_WORK_INITIALIZER(app_adc_work_handler),
	.adc = DEVICE_DT_GET(ADC_NODE),
	.ch_cfgs={DT_FOREACH_CHILD_SEP(ADC_NODE, ADC_CHANNEL_CFG_DT, (,))},
	.adc_seq_opt = {
		.extra_samplings = CONFIG_SEQUENCE_SAMPLES - 1,
		.interval_us = 10000,
		.callback = adc_seq_cb,
		.user_data = &ctx,
	},
	.seq = {
		.channels = BIT_MASK(DT_CHILD_NUM(ADC_NODE)),
		.buffer = ctx.channel_reading,
		.buffer_size = 	SIZEOF_FIELD(struct app_adc_ctx, channel_reading),
		.resolution = 16,
		.options = &ctx.adc_seq_opt,
	},
	.sem_stop = Z_SEM_INITIALIZER(ctx.sem_stop, 0, 1),
};

static void app_adc_work_handler(struct k_work *work)
{
	struct app_adc_ctx *ctx = CONTAINER_OF(work, struct app_adc_ctx, work);
	int err;
	if (!device_is_ready(ctx->adc)) {
		LOG_ERR("ADC controller device %s not ready\n", ctx->adc->name);
	}
	for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
		err = adc_channel_setup(ctx->adc, &ctx->ch_cfgs[i]);
		if (err < 0) {
			LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
		}
	}
	err = adc_read(ctx->adc, &ctx->seq);
	if (err < 0){
		LOG_ERR("adc_read() error (%d)\n", err);
	}
}
// void app_adc_thread(void *, void *, void *)
// {
// 	int err;
// 	while(1){
// 		err = k_sem_take(&app_adc_sem, K_FOREVER);
// 		if (err != 0) {
// 			LOG_ERR("k_sem_take() error: %d", err);
// 		}
// 		err = adc_read(adc, &sequence);
// 		if (err < 0){
// 			LOG_ERR("adc_read() error (%d)\n", err);
// 		}
		
// 	}
// }

// K_THREAD_DEFINE(adc_tid, APP_ADC_STACK_SIZE, app_adc_thread, NULL, NULL, NULL, APP_ADC_PRIORITY, 0, 0);

// #define MY_STACK_SIZE 512
// #define MY_PRIORITY 5


//K_WORK_DEFINE(app_adc_work, app_adc_work_handler);


static void gamepad_adc_ctrl_cb(const struct zbus_channel *chan)
{
	int err;
	const enum app_adc_action *action = zbus_chan_const_msg(chan);

	//TODO: perform start / stop actions
	if(*action == APP_ADC_ACTION_START){
		//check if the work is already busy, avoid resubmitting
		if(k_work_busy_get(&ctx.work)){
			LOG_INF("adc already working.");
		}
		else{
			err = k_work_submit_to_queue(&app_adc_work_q, &ctx.work);
			if(err<0){
				LOG_ERR("k_work_submit_to_queue() returns %d", err);
			}
		}
		return;
	}

	if(*action == APP_ADC_ACTION_STOP){
		k_sem_give(&ctx.sem_stop);
		return;
	}
}

ZBUS_LISTENER_DEFINE(gp_adc_ctrl_handler, gamepad_adc_ctrl_cb);

int app_adc_init(void)
{
    int err;
	// if (!device_is_ready(adc)) {
	// 	LOG_ERR("ADC controller device %s not ready\n", adc->name);
	// 	return 0;
	// }
	/* Configure channels individually prior to sampling. */
	// for (size_t i = 0U; i < CHANNEL_COUNT; i++) {
	// 	sequence.channels |= BIT(channel_cfgs[i].channel_id);
	// 	err = adc_channel_setup(adc, &channel_cfgs[i]);
	// 	if (err < 0) {
	// 		LOG_ERR("Could not setup channel #%d (%d)\n", i, err);
	// 		return 0;
	// 	}
	// 	// if ((vrefs_mv[i] == 0) && (channel_cfgs[i].reference == ADC_REF_INTERNAL)) {
	// 	// 	vrefs_mv[i] = adc_ref_internal(adc);
	// 	// }
	// }

	k_work_queue_init(&app_adc_work_q);
	k_work_queue_start(&app_adc_work_q, app_adc_workq_stack_area,K_THREAD_STACK_SIZEOF(app_adc_workq_stack_area), APP_ADC_PRIORITY,NULL);
}

int app_adc_start(gamepad_sample_ready_callback callback)
{
    int err;
    //err = adc_read(adc, &sequence);
}

int app_adc_stop(void)
{
	//how to stop 
}