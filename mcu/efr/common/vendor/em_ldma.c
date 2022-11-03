/***************************************************************************//**
 * @file
 * @brief Direct memory access (LDMA) module peripheral API
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#include "em_ldma.h"

#if defined(LDMA_PRESENT) && (LDMA_COUNT == 1)

#include <stddef.h>
#include "em_assert.h"
#include "em_bus.h"
#include "em_cmu.h"
#include "em_core.h"

/***************************************************************************//**
 * @addtogroup ldma
 * @{
 ******************************************************************************/

#if defined(LDMA_IRQ_HANDLER_TEMPLATE)
/***************************************************************************//**
 * @brief
 *   A template for an LDMA IRQ handler.
 ******************************************************************************/
void LDMA_IRQHandler(void)
{
  uint32_t ch;
  /* Get all pending and enabled interrupts. */
  uint32_t pending = LDMA_IntGetEnabled();

  /* Loop on an LDMA error to enable debugging. */
  while (pending & LDMA_IF_ERROR) {
  }

  /* Iterate over all LDMA channels. */
  for (ch = 0; ch < DMA_CHAN_COUNT; ch++) {
    uint32_t mask = 0x1 << ch;
    if (pending & mask) {
      /* Clear the interrupt flag. */
      LDMA->IFC = mask;

      /* Perform more actions here, execute callbacks, and so on. */
    }
  }
}
#endif

/***************************************************************************//**
 * @brief
 *   De-initialize the LDMA controller.
 *
 *   LDMA interrupts are disabled and the LDMA clock is stopped.
 ******************************************************************************/
void LDMA_DeInit(void)
{
  NVIC_DisableIRQ(LDMA_IRQn);
  LDMA->IEN  = 0;
#if defined(_LDMA_CHDIS_MASK)
  LDMA->CHDIS = _LDMA_CHEN_MASK;
#else
  LDMA->CHEN = 0;
#endif
#if defined(LDMA_EN_EN)
  LDMA->EN = 0;
#if defined(LDMA_EN_DISABLING)
  while (LDMA->EN & _LDMA_EN_DISABLING_MASK) {
  }
#endif
#endif

  CMU_ClockEnable(cmuClock_LDMA, false);
#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
  CMU_ClockEnable(cmuClock_LDMAXBAR, false);
#endif
}

/***************************************************************************//**
 * @brief
 *   Enable or disable an LDMA channel request.
 *
 * @details
 *   Use this function to enable or disable an LDMA channel request. This will
 *   prevent the LDMA from proceeding after its current transaction if disabled.
 *
 * @param[in] ch
 *   LDMA channel to enable or disable requests.
 *
 * @param[in] enable
 *   If 'true', the request will be enabled. If 'false', the request will be disabled.
 ******************************************************************************/
void LDMA_EnableChannelRequest(int ch, bool enable)
{
  EFM_ASSERT(ch < (int)DMA_CHAN_COUNT);

  BUS_RegBitWrite(&LDMA->REQDIS, ch, !enable);
}

/***************************************************************************//**
 * @brief
 *   Initialize the LDMA controller.
 *
 * @details
 *   This function will disable all the LDMA channels and enable the LDMA bus
 *   clock in the CMU. This function will also enable the LDMA IRQ in the NVIC
 *   and set the LDMA IRQ priority to a user-configurable priority. The LDMA
 *   interrupt priority is configured using the @ref LDMA_Init_t structure.
 *
 * @note
 *   Since this function enables the LDMA IRQ, always add a custom
 *   LDMA_IRQHandler to the application to handle any interrupts
 *   from LDMA.
 *
 * @param[in] init
 *   A pointer to the initialization structure used to configure the LDMA.
 ******************************************************************************/
void LDMA_Init(const LDMA_Init_t *init)
{
  uint32_t ldmaCtrlVal;
  EFM_ASSERT(init != NULL);
  EFM_ASSERT(!(((uint32_t)init->ldmaInitCtrlNumFixed << _LDMA_CTRL_NUMFIXED_SHIFT)
               & ~_LDMA_CTRL_NUMFIXED_MASK));

#if defined(_LDMA_CTRL_SYNCPRSCLREN_SHIFT) && defined (_LDMA_CTRL_SYNCPRSSETEN_SHIFT)
  EFM_ASSERT(!(((uint32_t)init->ldmaInitCtrlSyncPrsClrEn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)init->ldmaInitCtrlSyncPrsSetEn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));
#endif

#if defined(_LDMA_SYNCHWEN_SYNCCLREN_SHIFT) && defined (_LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
  EFM_ASSERT(!(((uint32_t)init->ldmaInitCtrlSyncPrsClrEn << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)init->ldmaInitCtrlSyncPrsSetEn << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCSETEN_MASK));
#endif

  EFM_ASSERT(init->ldmaInitIrqPriority < (1 << __NVIC_PRIO_BITS));

  CMU_ClockEnable(cmuClock_LDMA, true);
#if (_SILICON_LABS_32B_SERIES_2_CONFIG > 1)
  CMU_ClockEnable(cmuClock_LDMAXBAR, true);
#endif

#if defined(LDMA_EN_EN)
  LDMA->EN = LDMA_EN_EN;
#endif

  ldmaCtrlVal = (uint32_t)init->ldmaInitCtrlNumFixed << _LDMA_CTRL_NUMFIXED_SHIFT;

#if defined(_LDMA_CTRL_SYNCPRSCLREN_SHIFT) && defined (_LDMA_CTRL_SYNCPRSSETEN_SHIFT)
  ldmaCtrlVal |=  (init->ldmaInitCtrlSyncPrsClrEn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
                 | (init->ldmaInitCtrlSyncPrsSetEn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT);
#endif

  LDMA->CTRL = ldmaCtrlVal;

#if defined(_LDMA_SYNCHWEN_SYNCCLREN_SHIFT) && defined (_LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
  LDMA->SYNCHWEN = ((uint32_t)init->ldmaInitCtrlSyncPrsClrEn << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
                   | ((uint32_t)init->ldmaInitCtrlSyncPrsSetEn << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT);
#endif

#if defined(_LDMA_CHDIS_MASK)
  LDMA->CHDIS = _LDMA_CHEN_MASK;
#else
  LDMA->CHEN    = 0;
#endif
  LDMA->DBGHALT = 0;
  LDMA->REQDIS  = 0;

  /* Enable the LDMA error interrupt. */
  LDMA->IEN = LDMA_IEN_ERROR;
#if defined (LDMA_HAS_SET_CLEAR)
  LDMA->IF_CLR = 0xFFFFFFFFU;
#else
  LDMA->IFC = 0xFFFFFFFFU;
#endif
  NVIC_ClearPendingIRQ(LDMA_IRQn);

  /* Range is 0-7, where 0 is the highest priority. */
  NVIC_SetPriority(LDMA_IRQn, init->ldmaInitIrqPriority);

  NVIC_EnableIRQ(LDMA_IRQn);
}

/***************************************************************************//**
 * @brief
 *   Start a DMA transfer.
 *
 * @param[in] ch
 *   A DMA channel.
 *
 * @param[in] transfer
 *   The initialization structure used to configure the transfer.
 *
 * @param[in] descriptor
 *   The transfer descriptor, which can be an array of descriptors linked together.
 *   Each descriptor's fields stored in RAM will be loaded into the certain
 *   hardware registers at the proper time to perform the DMA transfer.
 ******************************************************************************/
void LDMA_StartTransfer(int ch,
                        const LDMA_TransferCfg_t *transfer,
                        const LDMA_Descriptor_t  *descriptor)
{
#if !(defined (_LDMA_SYNCHWEN_SYNCCLREN_SHIFT) && defined (_LDMA_SYNCHWEN_SYNCSETEN_SHIFT))
  uint32_t tmp;
#endif
  CORE_DECLARE_IRQ_STATE;
  uint32_t chMask = 1UL << (uint8_t)ch;

  EFM_ASSERT(ch < (int)DMA_CHAN_COUNT);
  EFM_ASSERT(transfer != NULL);

#if defined (_LDMAXBAR_CH_REQSEL_MASK)
  EFM_ASSERT(!(transfer->ldmaReqSel & ~_LDMAXBAR_CH_REQSEL_MASK));
#elif defined (_LDMA_CH_REQSEL_MASK)
  EFM_ASSERT(!(transfer->ldmaReqSel & ~_LDMA_CH_REQSEL_MASK));
#endif

#if defined (_LDMA_SYNCHWEN_SYNCCLREN_SHIFT) && defined (_LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsClrOff << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsClrOn << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsSetOff << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCSETEN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsSetOn << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT)
               & ~_LDMA_SYNCHWEN_SYNCSETEN_MASK));
#elif defined (_LDMA_CTRL_SYNCPRSCLREN_SHIFT) && defined (_LDMA_CTRL_SYNCPRSSETEN_SHIFT)
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsClrOff << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsClrOn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSCLREN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsSetOff << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCtrlSyncPrsSetOn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT)
               & ~_LDMA_CTRL_SYNCPRSSETEN_MASK));
#endif

  EFM_ASSERT(!(((uint32_t)transfer->ldmaCfgArbSlots << _LDMA_CH_CFG_ARBSLOTS_SHIFT)
               & ~_LDMA_CH_CFG_ARBSLOTS_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCfgSrcIncSign << _LDMA_CH_CFG_SRCINCSIGN_SHIFT)
               & ~_LDMA_CH_CFG_SRCINCSIGN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaCfgDstIncSign << _LDMA_CH_CFG_DSTINCSIGN_SHIFT)
               & ~_LDMA_CH_CFG_DSTINCSIGN_MASK));
  EFM_ASSERT(!(((uint32_t)transfer->ldmaLoopCnt << _LDMA_CH_LOOP_LOOPCNT_SHIFT)
               & ~_LDMA_CH_LOOP_LOOPCNT_MASK));

  /* Clear the pending channel interrupt. */
#if defined (LDMA_HAS_SET_CLEAR)
  LDMA->IF_CLR = chMask;
#else
  LDMA->IFC = chMask;
#endif

#if defined(LDMAXBAR)
  LDMAXBAR->CH[ch].REQSEL = transfer->ldmaReqSel;
#else
  LDMA->CH[ch].REQSEL = transfer->ldmaReqSel;
#endif
  LDMA->CH[ch].LOOP = transfer->ldmaLoopCnt << _LDMA_CH_LOOP_LOOPCNT_SHIFT;
  LDMA->CH[ch].CFG = (transfer->ldmaCfgArbSlots << _LDMA_CH_CFG_ARBSLOTS_SHIFT)
                     | (transfer->ldmaCfgSrcIncSign << _LDMA_CH_CFG_SRCINCSIGN_SHIFT)
                     | (transfer->ldmaCfgDstIncSign << _LDMA_CH_CFG_DSTINCSIGN_SHIFT)
#if defined(_LDMA_CH_CFG_SRCBUSPORT_MASK)
                     | (transfer->ldmaCfgStructBusPort << _LDMA_CH_CFG_STRUCTBUSPORT_SHIFT)
                     | (transfer->ldmaCfgSrcBusPort << _LDMA_CH_CFG_SRCBUSPORT_SHIFT)
                     | (transfer->ldmaCfgDstBusPort << _LDMA_CH_CFG_DSTBUSPORT_SHIFT)
#endif
  ;

  /* Set the descriptor address. */
  LDMA->CH[ch].LINK = (uint32_t)descriptor & _LDMA_CH_LINK_LINKADDR_MASK;

  /* A critical region. */
  CORE_ENTER_ATOMIC();

  /* Enable the channel interrupt. */
  LDMA->IEN |= chMask;

  if (transfer->ldmaReqDis) {
    LDMA->REQDIS |= chMask;
  }

  if (transfer->ldmaDbgHalt) {
    LDMA->DBGHALT |= chMask;
  }

#if defined (_LDMA_SYNCHWEN_SYNCCLREN_SHIFT) && defined (_LDMA_SYNCHWEN_SYNCSETEN_SHIFT)

  LDMA->SYNCHWEN_CLR =
    (((uint32_t)transfer->ldmaCtrlSyncPrsClrOff << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
     | ((uint32_t)transfer->ldmaCtrlSyncPrsSetOff << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT))
    & _LDMA_SYNCHWEN_MASK;

  LDMA->SYNCHWEN_SET =
    (((uint32_t)transfer->ldmaCtrlSyncPrsClrOn << _LDMA_SYNCHWEN_SYNCCLREN_SHIFT)
     | ((uint32_t)transfer->ldmaCtrlSyncPrsSetOn << _LDMA_SYNCHWEN_SYNCSETEN_SHIFT))
    & _LDMA_SYNCHWEN_MASK;

#elif defined (_LDMA_CTRL_SYNCPRSCLREN_SHIFT) && defined (_LDMA_CTRL_SYNCPRSSETEN_SHIFT)

  tmp = LDMA->CTRL;

  if (transfer->ldmaCtrlSyncPrsClrOff) {
    tmp &= ~_LDMA_CTRL_SYNCPRSCLREN_MASK
           | (~transfer->ldmaCtrlSyncPrsClrOff << _LDMA_CTRL_SYNCPRSCLREN_SHIFT);
  }

  if (transfer->ldmaCtrlSyncPrsClrOn) {
    tmp |= transfer->ldmaCtrlSyncPrsClrOn << _LDMA_CTRL_SYNCPRSCLREN_SHIFT;
  }

  if (transfer->ldmaCtrlSyncPrsSetOff) {
    tmp &= ~_LDMA_CTRL_SYNCPRSSETEN_MASK
           | (~transfer->ldmaCtrlSyncPrsSetOff << _LDMA_CTRL_SYNCPRSSETEN_SHIFT);
  }

  if (transfer->ldmaCtrlSyncPrsSetOn) {
    tmp |= transfer->ldmaCtrlSyncPrsSetOn << _LDMA_CTRL_SYNCPRSSETEN_SHIFT;
  }

  LDMA->CTRL = tmp;

#else

  #error  "SYNC Set and SYNC Clear not defined"

#endif

  BUS_RegMaskedClear(&LDMA->CHDONE, chMask);  /* Clear the done flag.     */
  LDMA->LINKLOAD = chMask;      /* Start a transfer by loading the descriptor.  */

  /* A critical region end. */
  CORE_EXIT_ATOMIC();
}

#if defined(_LDMA_CH_CTRL_EXTEND_MASK)
/***************************************************************************//**
 * @brief
 *   Start an extended DMA transfer.
 *
 * @param[in] ch
 *   A DMA channel.
 *
 * @param[in] transfer
 *   The initialization structure used to configure the transfer.
 *
 * @param[in] descriptor_ext
 *   The extended transfer descriptor, which can be an array of descriptors
 *   linked together. Each descriptor's fields stored in RAM will be loaded
 *   into the certain hardware registers at the proper time to perform the DMA
 *   transfer.
 ******************************************************************************/
void LDMA_StartTransferExtend(int ch,
                              const LDMA_TransferCfg_t *transfer,
                              const LDMA_DescriptorExtend_t *descriptor_ext)
{
  // Ensure destination interleaving supported for given channel.
  EFM_ASSERT(((1 << ch) & LDMA_ILCHNL));

  LDMA_StartTransfer(ch,
                     transfer,
                     (const LDMA_Descriptor_t *)descriptor_ext);
}
#endif

/***************************************************************************//**
 * @brief
 *   Stop a DMA transfer.
 *
 * @note
 *   The DMA will complete the current AHB burst transfer before stopping.
 *
 * @param[in] ch
 *   A DMA channel to stop.
 ******************************************************************************/
void LDMA_StopTransfer(int ch)
{
  uint32_t chMask = 1UL << (uint8_t)ch;

  EFM_ASSERT(ch < (int)DMA_CHAN_COUNT);

#if defined(_LDMA_CHDIS_MASK)
  CORE_ATOMIC_SECTION(
    LDMA->IEN &= ~chMask;
    LDMA->CHDIS = chMask;
    )
#else
  CORE_ATOMIC_SECTION(
    LDMA->IEN &= ~chMask;
    BUS_RegMaskedClear(&LDMA->CHEN, chMask);
    )
#endif
}

/***************************************************************************//**
 * @brief
 *   Check if a DMA transfer has completed.
 *
 * @param[in] ch
 *   A DMA channel to check.
 *
 * @return
 *   True if transfer has completed, false if not.
 ******************************************************************************/
bool LDMA_TransferDone(int ch)
{
  bool     retVal = false;
  uint32_t chMask = 1UL << (uint8_t)ch;

  EFM_ASSERT(ch < (int)DMA_CHAN_COUNT);

#if defined(_LDMA_CHSTATUS_MASK)
  CORE_ATOMIC_SECTION(
    if (((LDMA->CHSTATUS & chMask) == 0) && ((LDMA->CHDONE & chMask) == chMask)) {
    retVal = true;
  }
    )
#else
  CORE_ATOMIC_SECTION(
    if (((LDMA->CHEN & chMask) == 0) && ((LDMA->CHDONE & chMask) == chMask)) {
    retVal = true;
  }
    )
#endif

  return retVal;
}

/***************************************************************************//**
 * @brief
 *  Get the number of items remaining in a transfer.
 *
 * @note
 *  This function does not take into account that a DMA transfer with
 *  a chain of linked transfers might be ongoing. It will only check the
 *  count for the current transfer.
 *
 * @param[in] ch
 *  The channel number of the transfer to check.
 *
 * @return
 *  A number of items remaining in the transfer.
 ******************************************************************************/
uint32_t LDMA_TransferRemainingCount(int ch)
{
  uint32_t remaining, done, iflag;
  uint32_t chMask = 1UL << (uint8_t)ch;

  EFM_ASSERT(ch < (int)DMA_CHAN_COUNT);

  CORE_ATOMIC_SECTION(
    iflag  = LDMA->IF;
    done   = LDMA->CHDONE;
    remaining = LDMA->CH[ch].CTRL;
    )

  iflag    &= chMask;
  done     &= chMask;
  remaining = (remaining & _LDMA_CH_CTRL_XFERCNT_MASK)
              >> _LDMA_CH_CTRL_XFERCNT_SHIFT;

  if (done || ((remaining == 0) && iflag)) {
    return 0;
  }

  /* +1 because XFERCNT is 0-based. */
  return remaining + 1;
}

/** @} (end addtogroup ldma) */
#endif /* defined( LDMA_PRESENT ) && ( LDMA_COUNT == 1 ) */
