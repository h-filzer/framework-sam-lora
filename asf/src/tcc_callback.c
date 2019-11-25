/**
 * \file
 *
 * \brief SAM TCC - Timer Counter for Control Applications Callback Driver
 *
 * Copyright (c) 2013-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */

#include "tcc_callback.h"

#if TCC_ASYNC == true // WizIO

void *_tcc_instances[TCC_INST_NUM];

void _tcc_interrupt_handler(uint8_t module_index);

const uint32_t _tcc_intflag[TCC_CALLBACK_N] = {
	TCC_INTFLAG_OVF,
	TCC_INTFLAG_TRG,
	TCC_INTFLAG_CNT,
	TCC_INTFLAG_ERR,
	TCC_INTFLAG_FAULTA,
	TCC_INTFLAG_FAULTB,
	TCC_INTFLAG_FAULT0,
	TCC_INTFLAG_FAULT1,
#define _TCC_INTFLAG_MC(n,dummy) TCC_INTFLAG_MC##n,
	/* TCC_INTFLAG_MC0 ~ ... */
	MREPEAT(TCC_NUM_CHANNELS, _TCC_INTFLAG_MC, 0)
#undef _TCC_INTFLAG_MC
};

#  define _TCC_INTERRUPT_VECT_NUM(n, unused) \
		  SYSTEM_INTERRUPT_MODULE_TCC##n,
/**
 * \internal Get the interrupt vector for the given device instance
 *
 * \param[in] The TCC module instance number
 *
 * \return Interrupt vector for of the given TCC module instance.
 */
static enum system_interrupt_vector _tcc_interrupt_get_interrupt_vector(
		uint32_t inst_num)
{
	static uint8_t tcc_interrupt_vectors[TCC_INST_NUM] = {
		MREPEAT(TCC_INST_NUM, _TCC_INTERRUPT_VECT_NUM, 0)
	};

	return (enum system_interrupt_vector)tcc_interrupt_vectors[inst_num];
}

/**
 * \brief Registers a callback
 *
 * Registers a callback function which is implemented by the user.
 *
 * \note The callback must be enabled by \ref tcc_enable_callback,
 * in order for the interrupt handler to call it when the conditions for the
 * callback type is met.
 *
 * \param[in]     module        Pointer to TCC software instance struct
 * \param[in]     callback_func Pointer to callback function
 * \param[in]     callback_type Callback type given by an enum
 */
enum status_code tcc_register_callback(
		struct tcc_module *const module,
		tcc_callback_t callback_func,
		const enum tcc_callback callback_type)
{
	/* Sanity check arguments */
	Assert(module);
	Assert(callback_func);

	/* Register callback function */
	module->callback[callback_type] = callback_func;

	/* Set the bit corresponding to the callback_type */
	module->register_callback_mask |= _tcc_intflag[callback_type];

	return STATUS_OK;
}

/**
 * \brief Unregisters a callback
 *
 * Unregisters a callback function implemented by the user. The callback should
 * be disabled before it is unregistered.
 *
 * \param[in]     module Pointer to TCC software instance struct
 * \param[in]     callback_type Callback type given by an enum
 */
enum status_code tcc_unregister_callback(
		struct tcc_module *const module,
		const enum tcc_callback callback_type)
{
	/* Sanity check arguments */
	Assert(module);

	/* Unregister callback function */
	module->callback[callback_type] = NULL;

	/* Clear the bit corresponding to the callback_type */
	module->register_callback_mask &= ~_tcc_intflag[callback_type];

	return STATUS_OK;
}

/**
 * \brief Enables callback
 *
 * Enables the callback function registered by the \ref
 * tcc_register_callback. The callback function will be called from the
 * interrupt handler when the conditions for the callback type are
 * met. This function will also enable the appropriate interrupts.
 *
 * \param[in]     module Pointer to TCC software instance struct
 * \param[in]     callback_type Callback type given by an enum
 */
void tcc_enable_callback(
		struct tcc_module *const module,
		const enum tcc_callback callback_type)
{
	/* Sanity check arguments */
	Assert(module);
	Assert(module->hw);

	/* Enable interrupts for this TCC module */
	system_interrupt_enable(_tcc_interrupt_get_interrupt_vector(
			_tcc_get_inst_index(module->hw)));

	/* Enable channel or other callbacks */
	module->enable_callback_mask |= _tcc_intflag[callback_type];
	module->hw->INTENSET.reg = _tcc_intflag[callback_type];
}

/**
 * \brief Disables callback
 *
 * Disables the callback function registered by the \ref
 * tcc_register_callback, and the callback will not be called from the
 * interrupt routine. The function will also disable the appropriate
 * interrupts.
 *
 * \param[in]     module Pointer to TCC software instance struct
 * \param[in]     callback_type Callback type given by an enum
 */
void tcc_disable_callback(
		struct tcc_module *const module,
		const enum tcc_callback callback_type)
{
	/* Sanity check arguments */
	Assert(module);
	Assert(module->hw);

	/* Disable interrupts for this TCC module */
	system_interrupt_disable(_tcc_interrupt_get_interrupt_vector(
			_tcc_get_inst_index(module->hw)));

	/* Disable channel or other callbacks */
	module->enable_callback_mask &= ~_tcc_intflag[callback_type];
	module->hw->INTENCLR.reg = _tcc_intflag[callback_type];
}


/**
 * \internal ISR handler for TCC
 *
 * Auto-generate a set of interrupt handlers for each TCC in the device.
 */
#define _TCC_INTERRUPT_HANDLER(n, m) \
		void TCC##n##_Handler(void) \
		{ \
			_tcc_interrupt_handler(n); \
		}

MREPEAT(TCC_INST_NUM, _TCC_INTERRUPT_HANDLER, 0)

/**
 * \internal Interrupt handler for the TCC module
 *
 * Handles interrupts as they occur, it will run the callback functions
 * that are registered and enabled.
 *
 * \param[in]  module_index  ID of the TCC instance calling the interrupt
 *                           handler
 */
void _tcc_interrupt_handler(
		uint8_t module_index)
{
	int i;

	uint32_t interrupt_and_callback_status_mask;

	struct tcc_module *module =
			(struct tcc_module *)_tcc_instances[module_index];

	interrupt_and_callback_status_mask = (module->hw->INTFLAG.reg &
			module->register_callback_mask &
			module->enable_callback_mask);

	/* Check if callback interrupt has occured */
	for (i = 0; i < TCC_CALLBACK_N; i ++) {
		if (interrupt_and_callback_status_mask & _tcc_intflag[i]) {
			/* Invoke the registered and enabled callback function */
			(module->callback[i])(module);
			/* Clear interrupt flag */
			module->hw->INTFLAG.reg = _tcc_intflag[i];
		}
	}
}


#endif 