/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Josef Gajdusek
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * */

#ifndef LOG_H
#define LOG_H

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <CL/cl.h>

#include "utils.h"

#ifndef LOGLEVEL
#define LOGLEVEL 100
#endif

static char *log_ocl_errors[] = {
	[-CL_SUCCESS] = "CL_SUCCESS",
	[-CL_DEVICE_NOT_FOUND] = "CL_DEVICE_NOT_FOUND",
	[-CL_DEVICE_NOT_AVAILABLE] = "CL_DEVICE_NOT_AVAILABLE",
	[-CL_COMPILER_NOT_AVAILABLE] = "CL_COMPILER_NOT_AVAILABLE",
	[-CL_MEM_OBJECT_ALLOCATION_FAILURE] = "CL_MEM_OBJECT_ALLOCATION_FAILURE",
	[-CL_OUT_OF_RESOURCES] = "CL_OUT_OF_RESOURCES",
	[-CL_OUT_OF_HOST_MEMORY] = "CL_OUT_OF_HOST_MEMORY",
	[-CL_PROFILING_INFO_NOT_AVAILABLE] = "CL_PROFILING_INFO_NOT_AVAILABLE",
	[-CL_MEM_COPY_OVERLAP] = "CL_MEM_COPY_OVERLAP",
	[-CL_IMAGE_FORMAT_MISMATCH] = "CL_IMAGE_FORMAT_MISMATCH",
	[-CL_IMAGE_FORMAT_NOT_SUPPORTED] = "CL_IMAGE_FORMAT_NOT_SUPPORTED",
	[-CL_BUILD_PROGRAM_FAILURE] = "CL_BUILD_PROGRAM_FAILURE",
	[-CL_MAP_FAILURE] = "CL_MAP_FAILURE",
	[-CL_MISALIGNED_SUB_BUFFER_OFFSET] = "CL_MISALIGNED_SUB_BUFFER_OFFSET",
	[-CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST] = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST",
	[-CL_COMPILE_PROGRAM_FAILURE] = "CL_COMPILE_PROGRAM_FAILURE",
	[-CL_LINKER_NOT_AVAILABLE] = "CL_LINKER_NOT_AVAILABLE",
	[-CL_LINK_PROGRAM_FAILURE] = "CL_LINK_PROGRAM_FAILURE",
	[-CL_DEVICE_PARTITION_FAILED] = "CL_DEVICE_PARTITION_FAILED",
	[-CL_KERNEL_ARG_INFO_NOT_AVAILABLE] = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE",
	[-CL_INVALID_VALUE] = "CL_INVALID_VALUE",
	[-CL_INVALID_DEVICE_TYPE] = "CL_INVALID_DEVICE_TYPE",
	[-CL_INVALID_PLATFORM] = "CL_INVALID_PLATFORM",
	[-CL_INVALID_DEVICE] = "CL_INVALID_DEVICE",
	[-CL_INVALID_CONTEXT] = "CL_INVALID_CONTEXT",
	[-CL_INVALID_QUEUE_PROPERTIES] = "CL_INVALID_QUEUE_PROPERTIES",
	[-CL_INVALID_COMMAND_QUEUE] = "CL_INVALID_COMMAND_QUEUE",
	[-CL_INVALID_HOST_PTR] = "CL_INVALID_HOST_PTR",
	[-CL_INVALID_MEM_OBJECT] = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR",
	[-CL_INVALID_IMAGE_FORMAT_DESCRIPTOR] = "CL_INVALID_IMAGE_SIZE",
	[-CL_INVALID_IMAGE_SIZE] = "CL_INVALID_SAMPLER",
	[-CL_INVALID_SAMPLER] = "CL_INVALID_BINARY",
	[-CL_INVALID_BINARY] = "CL_INVALID_BUILD_OPTIONS",
	[-CL_INVALID_BUILD_OPTIONS] = "CL_INVALID_PROGRAM",
	[-CL_INVALID_PROGRAM] = "CL_INVALID_PROGRAM_EXECUTABLE",
	[-CL_INVALID_PROGRAM_EXECUTABLE] = "CL_INVALID_KERNEL_NAME",
	[-CL_INVALID_KERNEL_NAME] = "CL_INVALID_KERNEL_DEFINITION",
	[-CL_INVALID_KERNEL_DEFINITION] = "CL_INVALID_KERNEL",
	[-CL_INVALID_KERNEL] = "CL_INVALID_ARG_INDEX",
	[-CL_INVALID_ARG_VALUE] = "CL_INVALID_ARG_SIZE",
	[-CL_INVALID_KERNEL_ARGS] = "CL_INVALID_KERNEL_ARGS",
	[-CL_INVALID_WORK_DIMENSION] = "CL_INVALID_WORK_DIMENSION",
	[-CL_INVALID_WORK_GROUP_SIZE] = "CL_INVALID_WORK_GROUP_SIZE",
	[-CL_INVALID_WORK_ITEM_SIZE] = "CL_INVALID_WORK_ITEM_SIZE",
	[-CL_INVALID_GLOBAL_OFFSET] = "CL_INVALID_GLOBAL_OFFSET",
	[-CL_INVALID_EVENT_WAIT_LIST] = "CL_INVALID_EVENT_WAIT_LIST",
	[-CL_INVALID_EVENT] = "CL_INVALID_EVENT",
	[-CL_INVALID_OPERATION] = "CL_INVALID_OPERATION",
	[-CL_INVALID_GL_OBJECT] = "CL_INVALID_GL_OBJECT",
	[-CL_INVALID_BUFFER_SIZE] = "CL_INVALID_BUFFER_SIZE",
	[-CL_INVALID_MIP_LEVEL] = "CL_INVALID_MIP_LEVEL",
	[-CL_INVALID_GLOBAL_WORK_SIZE] = "CL_INVALID_GLOBAL_WORK_SIZE",
	[-CL_INVALID_PROPERTY] = "CL_INVALID_PROPERTY",
	[-CL_INVALID_IMAGE_DESCRIPTOR] = "CL_INVALID_IMAGE_DESCRIPTOR",
	[-CL_INVALID_COMPILER_OPTIONS] = "CL_INVALID_COMPILER_OPTIONS",
	[-CL_INVALID_LINKER_OPTIONS] = "CL_INVALID_LINKER_OPTIONS",
	[-CL_INVALID_DEVICE_PARTITION_COUNT] = "CL_INVALID_DEVICE_PARTITION_COUNT",
};

static inline char *log_ocl_errstr(cl_int err)
{
	if (err < 0 || err >= (int)ARRAY_SIZE(log_ocl_errors)) {
		return "Unknown error";
	}

	return log_ocl_errors[err];
}

#define log_write(color, level, format, args...) \
	fprintf(stderr, color "[" level "]:%s():%d: " format "\x1b[0m\n", __func__, __LINE__ , ##args)

#if LOGLEVEL >= 0
#define log_error(format, args...) log_write("\e[1;31m", "ERR", format, ##args)
#define log_error_errno(format, args...) log_error(format ": %s (%d)", ##args, strerror(errno), errno)
#define log_error_clerr(format, code, args...) log_error(format ": %s (%d)", ##args, log_ocl_errstr(-code), code)
#else
#define log_error(format, args...)
#define log_error_errno(format, args...)
#endif

#if LOGLEVEL >= 1
#define log_warn(format, args...) log_write("\x1b[1;33m", "WARN", format, ##args)
#else
#define log_warn(format, args...)
#endif

#if LOGLEVEL >= 2
#define log_info(format, args...) log_write("\x1b[1;34m", "INFO", format, ##args)
#else
#define log_info(format, args...)
#endif

#if LOGLEVEL >= 3
#define log_debug(format, args...) log_write("\x1b[0;37m", "DEBUG", format, ##args)
#else
#define log_debug(format, args...)
#endif

#endif
