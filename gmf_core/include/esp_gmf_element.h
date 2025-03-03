/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2024 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "esp_gmf_err.h"
#include "esp_gmf_obj.h"
#include "esp_gmf_job.h"
#include "esp_gmf_event.h"
#include "esp_gmf_info.h"
#include "esp_gmf_port.h"
#include "esp_gmf_method.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define ESP_GMF_ELEMENT_JOB_OPEN          BIT(0)
#define ESP_GMF_ELEMENT_JOB_PROCESS       BIT(1)
#define ESP_GMF_ELEMENT_JOB_CLOSE         BIT(2)
#define ESP_GMF_ELEMENT_PORT_DATA_SIZE_DEFAULT (768)
#define ESP_GMF_ELEMENT_PORT_ADDR_ALIGNED_DEFAULT (16)  //  16 bytes aligned

#define ESP_GMF_MAX_DELAY (0xFFFFFFFFUL)

#define ESP_GMF_ELEMENT_GET(x)            ((esp_gmf_element_t *)x)
#define ESP_GMF_ELEMENT_GET_IN_PORT(x)    (((esp_gmf_element_t *)x)->in)
#define ESP_GMF_ELEMENT_GET_OUT_PORT(x)   (((esp_gmf_element_t *)x)->out)
#define ESP_GMF_ELEMENT_GET_DEPENDENCY(x) (((esp_gmf_element_t *)x)->dependency)

#define ESP_GMF_ELEMENT_IN_PORT_ATTR_SET(attr, caps, addr_aligned, size_aligned, port_type, acq_data_size) do {  \
    (attr).cap = (uint8_t)(caps);                                                                                \
    (attr).port.buf_addr_aligned = (uint8_t)(addr_aligned);                                                      \
    (attr).port.buf_size_aligned = (uint8_t)(size_aligned);                                                      \
    (attr).port.dir = (ESP_GMF_PORT_DIR_IN);                                                                     \
    (attr).port.type = (uint8_t)(port_type);                                                                     \
    (attr).data_size = (int)(acq_data_size);                                                                     \
} while (0)

#define ESP_GMF_ELEMENT_OUT_PORT_ATTR_SET(attr, caps, addr_aligned, size_aligned, port_type, acq_data_size) do {  \
    (attr).cap = (uint8_t)(caps);                                                                                 \
    (attr).port.buf_addr_aligned = (uint8_t)(addr_aligned);                                                       \
    (attr).port.buf_size_aligned = (uint8_t)(size_aligned);                                                       \
    (attr).port.dir = (ESP_GMF_PORT_DIR_OUT);                                                                     \
    (attr).port.type = (uint8_t)(port_type);                                                                      \
    (attr).data_size = (int)(acq_data_size);                                                                      \
} while (0)

/**
 * @brief  Defining the bit mask for an element's port capabilities
 */
#define ESP_GMF_EL_PORT_CAP_SINGLE (1)  /*!< Bit0 for single port capability */
#define ESP_GMF_EL_PORT_CAP_MULTI (2)   /*!< Bit1 for multi-port capability */

/**
 * @brief  The GMF element handle
 */
typedef void *esp_gmf_element_handle_t;

/**
 * @brief  Structure defining the attributes of an element's port
 */
typedef struct {
    uint8_t              cap;        /*!< An element can connect to one or more capability ports */
    esp_gmf_port_attr_t  port;       /*!< Port attributes */
    int                  data_size;  /*!< A minimum data size for element acquisition operations,
                                          recommended for all elements, even those without specific processing requirements */
} esp_gmf_element_port_attr_t;

/**
 * @brief  Structure defining the operations of an element
 */
typedef struct {
    esp_gmf_job_func  open;            /*!< Function to open the element */
    esp_gmf_job_func  process;         /*!< Function to process the element */
    esp_gmf_job_func  close;           /*!< Function to close the element */
    esp_gmf_event_cb  event_receiver;  /*!< Event receiver function */
} esp_gmf_element_ops_t;

/**
 * @brief  Structure representing a GMF element
 */
typedef struct esp_gmf_element {
    esp_gmf_obj_t                   base;           /*!< Base object */
    esp_gmf_element_ops_t           ops;            /*!< Operations */
    uint8_t                         job_mask;       /*!< Job mask */

    esp_gmf_port_t                 *in;             /*!< Input port */
    esp_gmf_element_port_attr_t     in_attr;        /*!< Input port attributes */

    esp_gmf_port_t                 *out;            /*!< Output port */
    esp_gmf_element_port_attr_t     out_attr;       /*!< Output port attributes */

    /* Properties */
    esp_gmf_event_state_t           init_state;     /*!< Initial state */
    esp_gmf_event_state_t           cur_state;      /*!< Current state */
    esp_gmf_event_cb                event_func;     /*!< Event function */
    esp_gmf_method_t               *method;         /*!< It can access the data members and member functions of the objects */

    /* Protect */
    void                           *ctx;            /*!< User Context */
    uint8_t                         dependency : 1; /*!< Indicates if the element depends on other information to open */
} esp_gmf_element_t;

/**
 * @brief  Configuration structure for a GMF element
 */
typedef struct {
    void                           *ctx;         /*!< User context */
    esp_gmf_event_cb                cb;          /*!< Callback function */
    esp_gmf_element_port_attr_t     in_attr;     /*!< Input port attributes */
    esp_gmf_element_port_attr_t     out_attr;    /*!< Output port attributes */
    bool                            dependency;  /*!< Indicates if the element depends on other information to open */
} esp_gmf_element_cfg_t;

/**
 * @brief  Initialize the given element with the configuration
 *
 * @param[in]  handle  GMF element handle to initialize
 * @param[in]  config  Pointer to the configuration structure
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_init(esp_gmf_element_handle_t handle, esp_gmf_element_cfg_t *config);

/**
 * @brief  Deinitialize the specific element, freeing associated resources
 *
 * @param[in]  handle  GMF element handle to deinitialize
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_deinit(esp_gmf_element_handle_t handle);

/**
 * @brief  Set the event callback function for the specific element
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  cb      Event callback function
 * @param[in]  ctx     Context for the callback function
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_set_event_func(esp_gmf_element_handle_t handle, esp_gmf_event_cb cb, void *ctx);

/**
 * @brief  Register an input port for the specific element
 *
 * @param[in]  handle   GMF element handle
 * @param[in]  io_inst  port handle to register
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid arguments
 *       - ESP_GMF_ERR_NOT_SUPPORT  The specified port type is not supported, or the element can't connect more port
 */
esp_gmf_err_t esp_gmf_element_register_in_port(esp_gmf_element_handle_t handle, esp_gmf_port_handle_t io_inst);

/**
 * @brief  Unregister an input port from the specific element
 *         If `io_inst` is NULL, it unregisters all ports of the element
 *
 * @param[in]  handle   GMF element handle
 * @param[in]  io_inst  Input port handle to unregister
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 *       - ESP_GMF_ERR_NOT_FOUND    The specified port was not found
 */
esp_gmf_err_t esp_gmf_element_unregister_in_port(esp_gmf_element_handle_t handle, esp_gmf_port_handle_t io_inst);

/**
 * @brief  Register an output port for the specific element
 *
 * @param[in]  handle   GMF element handle
 * @param[in]  io_inst  Output port handle to register
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid arguments
 *       - ESP_GMF_ERR_NOT_SUPPORT  The specified port type is not supported, or the element can't connect more port
 */
esp_gmf_err_t esp_gmf_element_register_out_port(esp_gmf_element_handle_t handle, esp_gmf_port_handle_t io_inst);

/**
 * @brief  Unregister an output port from the specific element
 *         If `io_inst` is NULL, it unregisters all ports of the element
 *
 * @param[in]  handle   GMF element handle
 * @param[in]  io_inst  Output port handle to unregister
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 *       - ESP_GMF_ERR_NOT_FOUND    The specified port was not found
 */
esp_gmf_err_t esp_gmf_element_unregister_out_port(esp_gmf_element_handle_t handle, esp_gmf_port_handle_t io_inst);

/**
 * @brief  Link a new element to the given element
 *
 * @param[in]  handle  Given element handle
 * @param[in]  new_el  New element handle to link
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_link_el(esp_gmf_element_handle_t handle, esp_gmf_element_handle_t new_el);

/**
 * @brief  Get the next linked element by the given element
 *
 * @param[in]  handle   Given element handle
 * @param[in]  next_el  Next element handle
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_get_next_el(esp_gmf_element_handle_t handle, esp_gmf_element_handle_t *next_el);

/**
 * @brief  Get the previous linked element by the given element
 *
 * @param[in]  handle   Given element handle
 * @param[in]  prev_el  Previous element handle
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument
 */
esp_gmf_err_t esp_gmf_element_get_prev_el(esp_gmf_element_handle_t handle, esp_gmf_element_handle_t *prev_el);

/**
 * @brief  Process the open phase for the specific element
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  para    Pointer to the parameters for processing
 *
 * @return
 *       - ESP_GMF_JOB_ERR_DONE      Indicating the job has been completed
 *       - ESP_GMF_JOB_ERR_CONTINUE  Indicating the job should continue
 *       - ESP_GMF_JOB_ERR_OK        Indicating the job has executed successfully
 *       - ESP_GMF_JOB_ERR_FAIL      Indicating the job has failed to execute
 */
esp_gmf_job_err_t esp_gmf_element_process_open(esp_gmf_element_handle_t handle, void *para);

/**
 * @brief  Process the close phase for the specific element
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  para    Pointer to the parameters for processing
 *
 * @return
 *       - ESP_GMF_JOB_ERR_DONE      Indicating the job has been completed
 *       - ESP_GMF_JOB_ERR_CONTINUE  Indicating the job should continue
 *       - ESP_GMF_JOB_ERR_OK        Indicating the job has executed successfully
 *       - ESP_GMF_JOB_ERR_FAIL      Indicating the job has failed to execute
 */
esp_gmf_job_err_t esp_gmf_element_process_close(esp_gmf_element_handle_t handle, void *para);

/**
 * @brief  Process the running phase for the specific element
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  para    Pointer to the parameters for processing
 *
 * @return
 *       - ESP_GMF_JOB_ERR_DONE      Indicating the job has been completed
 *       - ESP_GMF_JOB_ERR_CONTINUE  Indicating the job should continue
 *       - ESP_GMF_JOB_ERR_OK        Indicating the job has executed successfully
 *       - ESP_GMF_JOB_ERR_FAIL      Indicating the job has failed to execute
 */
esp_gmf_job_err_t esp_gmf_element_process_running(esp_gmf_element_handle_t handle, void *para);

/**
 * @brief  Set the state of the specific element
 *
 * @param[in]  handle     GMF element handle
 * @param[in]  new_state  New state to set
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_set_state(esp_gmf_element_handle_t handle, esp_gmf_event_state_t new_state);

/**
 * @brief  Get the state of the specific element
 *
 * @param[in]   handle  GMF element handle
 * @param[out]  state   Pointer to store the current state
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_get_state(esp_gmf_element_handle_t handle, esp_gmf_event_state_t *state);

/**
 * @brief  Reset the state of the specific element to its initial state and clean the `job_mask`
 *
 * @param[in]  handle  GMF element handle
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_reset_state(esp_gmf_element_handle_t handle);

/**
 * @brief  Reset the ports of the specific element to their initial state
 *
 * @param[in]  handle  GMF element handle
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_reset_port(esp_gmf_element_handle_t handle);

/**
 * @brief  Receive an event packet for the specific element
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  event   Pointer to the event packet
 * @param[in]  ctx     Context for event processing
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_receive_event(esp_gmf_element_handle_t handle, esp_gmf_event_pkt_t *event, void *ctx);

/**
 * @brief  Set the job mask of the GMF element to the given mask value
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  mask    Job mask to set
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_set_job_mask(esp_gmf_element_handle_t handle, uint16_t mask);

/**
 * @brief  Update the job mask for the specific element by performing a bitwise OR operation with the given mask value
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  mask    Job mask to apply
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_change_job_mask(esp_gmf_element_handle_t handle, uint16_t mask);

/**
 * @brief  Get the job mask for the specific element
 *
 * @param[in]   handle  GMF element handle
 * @param[out]  mask    Pointer to store the job mask
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 */
esp_gmf_err_t esp_gmf_element_get_job_mask(esp_gmf_element_handle_t handle, uint16_t *mask);

/**
 * @brief  Notify the specific element about sound information
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  info    Pointer to sound information
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_FAIL         No event callback function
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 *       - Others                   Failed
 */
esp_gmf_err_t esp_gmf_element_notify_snd_info(esp_gmf_element_handle_t handle, esp_gmf_info_sound_t *info);

/**
 * @brief  Notify the specific element about video information
 *
 * @param[in]  handle  GMF element handle
 * @param[in]  info    Pointer to video information
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_FAIL         No event callback function
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle is invalid
 *       - Others                   Failed
 */
esp_gmf_err_t esp_gmf_element_notify_vid_info(esp_gmf_element_handle_t handle, esp_gmf_info_video_t *info);

/**
 * @brief  Register a method for a GMF element
 *
 *         This function registers a method identified by `id` with the GMF element specified by `handle`
 *         The method will be associated with the function pointer `func`
 *
 * @param[in]  handle     Handle to the GMF element where the method will be registered
 * @param[in]  name       Name for the method to be registered
 * @param[in]  func       Function pointer to the method implementation
 * @param[in]  args_desc  A pointer to the argument description
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_MEMORY_LACK  No enogth memory
 *       - ESP_GMF_ERR_INVALID_ARG  If the handle or func is invalid
 */
esp_gmf_err_t esp_gmf_element_register_method(esp_gmf_element_handle_t handle, const char *name,
                                              esp_gmf_method_func func, esp_gmf_args_desc_t *args_desc);

/**
 * @brief  Execute methods of GMF element by argument list
 *
 * @param[in]  handle   Pointer to the handle of the GMF element
 * @param[in]  name     The name of the method to be executed
 * @param[in]  buf      Pointer to the buffer containing the arguments for the method
 * @param[in]  buf_len  The length of the buffer (`buf`)
 *
 * @return
 *       - ESP_GMF_ERR_OK           On success
 *       - ESP_GMF_ERR_NOT_SUPPORT  No executable methods found for the given name
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid handle or arguments
 */
esp_gmf_err_t esp_gmf_element_exe_method(esp_gmf_element_handle_t handle, const char *name,
                                                  uint8_t *buf, int buf_len);

/**
 * @brief  Retrieve the method structure associated with a given ESP-GMF element
 *
 * @param[in]   handle  Pointer to the ESP-GMF element handle whose method structure is to be retrieved
 * @param[out]  mthd    Pointer to a pointer where the address of the method structure will be stored upon successful retrieval
 *
 * @return
 *       - ESP_GMF_OK               Method structure retrieved successfully
 *       - ESP_GMF_ERR_INVALID_ARG  Invalid argument, such as a NULL handle or output pointer
 */
esp_gmf_err_t esp_gmf_element_get_method(esp_gmf_element_handle_t handle, esp_gmf_method_t **mthd);

#ifdef __cplusplus
}
#endif  /* __cplusplus */
