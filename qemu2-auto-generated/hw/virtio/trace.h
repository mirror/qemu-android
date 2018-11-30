/* This file is autogenerated by tracetool, do not edit. */

#ifndef TRACE_HW_VIRTIO_GENERATED_TRACERS_H
#define TRACE_HW_VIRTIO_GENERATED_TRACERS_H

#include "qemu-common.h"
#include "trace/control.h"

extern TraceEvent _TRACE_VHOST_COMMIT_EVENT;
extern TraceEvent _TRACE_VHOST_REGION_ADD_SECTION_EVENT;
extern TraceEvent _TRACE_VHOST_REGION_ADD_SECTION_MERGE_EVENT;
extern TraceEvent _TRACE_VHOST_REGION_ADD_SECTION_ALIGNED_EVENT;
extern TraceEvent _TRACE_VHOST_SECTION_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_END_ENTRY_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_END_EXIT_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_LOOP_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_FOUND_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_LISTEN_EVENT;
extern TraceEvent _TRACE_VHOST_USER_SET_MEM_TABLE_POSTCOPY_EVENT;
extern TraceEvent _TRACE_VHOST_USER_SET_MEM_TABLE_WITHFD_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_WAKER_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_WAKER_FOUND_EVENT;
extern TraceEvent _TRACE_VHOST_USER_POSTCOPY_WAKER_NOMATCH_EVENT;
extern TraceEvent _TRACE_VIRTQUEUE_ALLOC_ELEMENT_EVENT;
extern TraceEvent _TRACE_VIRTQUEUE_FILL_EVENT;
extern TraceEvent _TRACE_VIRTQUEUE_FLUSH_EVENT;
extern TraceEvent _TRACE_VIRTQUEUE_POP_EVENT;
extern TraceEvent _TRACE_VIRTIO_QUEUE_NOTIFY_EVENT;
extern TraceEvent _TRACE_VIRTIO_NOTIFY_IRQFD_EVENT;
extern TraceEvent _TRACE_VIRTIO_NOTIFY_EVENT;
extern TraceEvent _TRACE_VIRTIO_SET_STATUS_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_GUEST_NOT_READY_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_CPU_IS_STOPPED_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_POPPED_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_PUSHED_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_REQUEST_EVENT;
extern TraceEvent _TRACE_VIRTIO_RNG_VM_STATE_CHANGE_EVENT;
extern TraceEvent _TRACE_VIRTIO_BALLOON_BAD_ADDR_EVENT;
extern TraceEvent _TRACE_VIRTIO_BALLOON_HANDLE_OUTPUT_EVENT;
extern TraceEvent _TRACE_VIRTIO_BALLOON_GET_CONFIG_EVENT;
extern TraceEvent _TRACE_VIRTIO_BALLOON_SET_CONFIG_EVENT;
extern TraceEvent _TRACE_VIRTIO_BALLOON_TO_TARGET_EVENT;
extern uint16_t _TRACE_VHOST_COMMIT_DSTATE;
extern uint16_t _TRACE_VHOST_REGION_ADD_SECTION_DSTATE;
extern uint16_t _TRACE_VHOST_REGION_ADD_SECTION_MERGE_DSTATE;
extern uint16_t _TRACE_VHOST_REGION_ADD_SECTION_ALIGNED_DSTATE;
extern uint16_t _TRACE_VHOST_SECTION_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_END_ENTRY_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_END_EXIT_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_LOOP_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_FOUND_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_LISTEN_DSTATE;
extern uint16_t _TRACE_VHOST_USER_SET_MEM_TABLE_POSTCOPY_DSTATE;
extern uint16_t _TRACE_VHOST_USER_SET_MEM_TABLE_WITHFD_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_WAKER_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_WAKER_FOUND_DSTATE;
extern uint16_t _TRACE_VHOST_USER_POSTCOPY_WAKER_NOMATCH_DSTATE;
extern uint16_t _TRACE_VIRTQUEUE_ALLOC_ELEMENT_DSTATE;
extern uint16_t _TRACE_VIRTQUEUE_FILL_DSTATE;
extern uint16_t _TRACE_VIRTQUEUE_FLUSH_DSTATE;
extern uint16_t _TRACE_VIRTQUEUE_POP_DSTATE;
extern uint16_t _TRACE_VIRTIO_QUEUE_NOTIFY_DSTATE;
extern uint16_t _TRACE_VIRTIO_NOTIFY_IRQFD_DSTATE;
extern uint16_t _TRACE_VIRTIO_NOTIFY_DSTATE;
extern uint16_t _TRACE_VIRTIO_SET_STATUS_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_GUEST_NOT_READY_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_CPU_IS_STOPPED_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_POPPED_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_PUSHED_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_REQUEST_DSTATE;
extern uint16_t _TRACE_VIRTIO_RNG_VM_STATE_CHANGE_DSTATE;
extern uint16_t _TRACE_VIRTIO_BALLOON_BAD_ADDR_DSTATE;
extern uint16_t _TRACE_VIRTIO_BALLOON_HANDLE_OUTPUT_DSTATE;
extern uint16_t _TRACE_VIRTIO_BALLOON_GET_CONFIG_DSTATE;
extern uint16_t _TRACE_VIRTIO_BALLOON_SET_CONFIG_DSTATE;
extern uint16_t _TRACE_VIRTIO_BALLOON_TO_TARGET_DSTATE;
#define TRACE_VHOST_COMMIT_ENABLED 1
#define TRACE_VHOST_REGION_ADD_SECTION_ENABLED 1
#define TRACE_VHOST_REGION_ADD_SECTION_MERGE_ENABLED 1
#define TRACE_VHOST_REGION_ADD_SECTION_ALIGNED_ENABLED 1
#define TRACE_VHOST_SECTION_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_END_ENTRY_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_END_EXIT_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_LOOP_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_FOUND_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_LISTEN_ENABLED 1
#define TRACE_VHOST_USER_SET_MEM_TABLE_POSTCOPY_ENABLED 1
#define TRACE_VHOST_USER_SET_MEM_TABLE_WITHFD_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_WAKER_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_WAKER_FOUND_ENABLED 1
#define TRACE_VHOST_USER_POSTCOPY_WAKER_NOMATCH_ENABLED 1
#define TRACE_VIRTQUEUE_ALLOC_ELEMENT_ENABLED 1
#define TRACE_VIRTQUEUE_FILL_ENABLED 1
#define TRACE_VIRTQUEUE_FLUSH_ENABLED 1
#define TRACE_VIRTQUEUE_POP_ENABLED 1
#define TRACE_VIRTIO_QUEUE_NOTIFY_ENABLED 1
#define TRACE_VIRTIO_NOTIFY_IRQFD_ENABLED 1
#define TRACE_VIRTIO_NOTIFY_ENABLED 1
#define TRACE_VIRTIO_SET_STATUS_ENABLED 1
#define TRACE_VIRTIO_RNG_GUEST_NOT_READY_ENABLED 1
#define TRACE_VIRTIO_RNG_CPU_IS_STOPPED_ENABLED 1
#define TRACE_VIRTIO_RNG_POPPED_ENABLED 1
#define TRACE_VIRTIO_RNG_PUSHED_ENABLED 1
#define TRACE_VIRTIO_RNG_REQUEST_ENABLED 1
#define TRACE_VIRTIO_RNG_VM_STATE_CHANGE_ENABLED 1
#define TRACE_VIRTIO_BALLOON_BAD_ADDR_ENABLED 1
#define TRACE_VIRTIO_BALLOON_HANDLE_OUTPUT_ENABLED 1
#define TRACE_VIRTIO_BALLOON_GET_CONFIG_ENABLED 1
#define TRACE_VIRTIO_BALLOON_SET_CONFIG_ENABLED 1
#define TRACE_VIRTIO_BALLOON_TO_TARGET_ENABLED 1

#define TRACE_VHOST_COMMIT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_commit(bool started, bool changed)
{
}

static inline void trace_vhost_commit(bool started, bool changed)
{
    if (true) {
        _nocheck__trace_vhost_commit(started, changed);
    }
}

#define TRACE_VHOST_REGION_ADD_SECTION_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_region_add_section(const char * name, uint64_t gpa, uint64_t size, uint64_t host)
{
}

static inline void trace_vhost_region_add_section(const char * name, uint64_t gpa, uint64_t size, uint64_t host)
{
    if (true) {
        _nocheck__trace_vhost_region_add_section(name, gpa, size, host);
    }
}

#define TRACE_VHOST_REGION_ADD_SECTION_MERGE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_region_add_section_merge(const char * name, uint64_t new_size, uint64_t gpa, uint64_t owr)
{
}

static inline void trace_vhost_region_add_section_merge(const char * name, uint64_t new_size, uint64_t gpa, uint64_t owr)
{
    if (true) {
        _nocheck__trace_vhost_region_add_section_merge(name, new_size, gpa, owr);
    }
}

#define TRACE_VHOST_REGION_ADD_SECTION_ALIGNED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_region_add_section_aligned(const char * name, uint64_t gpa, uint64_t size, uint64_t host)
{
}

static inline void trace_vhost_region_add_section_aligned(const char * name, uint64_t gpa, uint64_t size, uint64_t host)
{
    if (true) {
        _nocheck__trace_vhost_region_add_section_aligned(name, gpa, size, host);
    }
}

#define TRACE_VHOST_SECTION_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_section(const char * name, int r)
{
}

static inline void trace_vhost_section(const char * name, int r)
{
    if (true) {
        _nocheck__trace_vhost_section(name, r);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_END_ENTRY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_end_entry(void)
{
}

static inline void trace_vhost_user_postcopy_end_entry(void)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_end_entry();
    }
}

#define TRACE_VHOST_USER_POSTCOPY_END_EXIT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_end_exit(void)
{
}

static inline void trace_vhost_user_postcopy_end_exit(void)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_end_exit();
    }
}

#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_fault_handler(const char * name, uint64_t fault_address, int nregions)
{
}

static inline void trace_vhost_user_postcopy_fault_handler(const char * name, uint64_t fault_address, int nregions)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_fault_handler(name, fault_address, nregions);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_LOOP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_fault_handler_loop(int i, uint64_t client_base, uint64_t size)
{
}

static inline void trace_vhost_user_postcopy_fault_handler_loop(int i, uint64_t client_base, uint64_t size)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_fault_handler_loop(i, client_base, size);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_FAULT_HANDLER_FOUND_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_fault_handler_found(int i, uint64_t region_offset, uint64_t rb_offset)
{
}

static inline void trace_vhost_user_postcopy_fault_handler_found(int i, uint64_t region_offset, uint64_t rb_offset)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_fault_handler_found(i, region_offset, rb_offset);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_LISTEN_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_listen(void)
{
}

static inline void trace_vhost_user_postcopy_listen(void)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_listen();
    }
}

#define TRACE_VHOST_USER_SET_MEM_TABLE_POSTCOPY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_set_mem_table_postcopy(uint64_t client_addr, uint64_t qhva, int reply_i, int region_i)
{
}

static inline void trace_vhost_user_set_mem_table_postcopy(uint64_t client_addr, uint64_t qhva, int reply_i, int region_i)
{
    if (true) {
        _nocheck__trace_vhost_user_set_mem_table_postcopy(client_addr, qhva, reply_i, region_i);
    }
}

#define TRACE_VHOST_USER_SET_MEM_TABLE_WITHFD_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_set_mem_table_withfd(int index, const char * name, uint64_t memory_size, uint64_t guest_phys_addr, uint64_t userspace_addr, uint64_t offset)
{
}

static inline void trace_vhost_user_set_mem_table_withfd(int index, const char * name, uint64_t memory_size, uint64_t guest_phys_addr, uint64_t userspace_addr, uint64_t offset)
{
    if (true) {
        _nocheck__trace_vhost_user_set_mem_table_withfd(index, name, memory_size, guest_phys_addr, userspace_addr, offset);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_WAKER_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_waker(const char * rb, uint64_t rb_offset)
{
}

static inline void trace_vhost_user_postcopy_waker(const char * rb, uint64_t rb_offset)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_waker(rb, rb_offset);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_WAKER_FOUND_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_waker_found(uint64_t client_addr)
{
}

static inline void trace_vhost_user_postcopy_waker_found(uint64_t client_addr)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_waker_found(client_addr);
    }
}

#define TRACE_VHOST_USER_POSTCOPY_WAKER_NOMATCH_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_vhost_user_postcopy_waker_nomatch(const char * rb, uint64_t rb_offset)
{
}

static inline void trace_vhost_user_postcopy_waker_nomatch(const char * rb, uint64_t rb_offset)
{
    if (true) {
        _nocheck__trace_vhost_user_postcopy_waker_nomatch(rb, rb_offset);
    }
}

#define TRACE_VIRTQUEUE_ALLOC_ELEMENT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtqueue_alloc_element(void * elem, size_t sz, unsigned in_num, unsigned out_num)
{
}

static inline void trace_virtqueue_alloc_element(void * elem, size_t sz, unsigned in_num, unsigned out_num)
{
    if (true) {
        _nocheck__trace_virtqueue_alloc_element(elem, sz, in_num, out_num);
    }
}

#define TRACE_VIRTQUEUE_FILL_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtqueue_fill(void * vq, const void * elem, unsigned int len, unsigned int idx)
{
}

static inline void trace_virtqueue_fill(void * vq, const void * elem, unsigned int len, unsigned int idx)
{
    if (true) {
        _nocheck__trace_virtqueue_fill(vq, elem, len, idx);
    }
}

#define TRACE_VIRTQUEUE_FLUSH_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtqueue_flush(void * vq, unsigned int count)
{
}

static inline void trace_virtqueue_flush(void * vq, unsigned int count)
{
    if (true) {
        _nocheck__trace_virtqueue_flush(vq, count);
    }
}

#define TRACE_VIRTQUEUE_POP_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtqueue_pop(void * vq, void * elem, unsigned int in_num, unsigned int out_num)
{
}

static inline void trace_virtqueue_pop(void * vq, void * elem, unsigned int in_num, unsigned int out_num)
{
    if (true) {
        _nocheck__trace_virtqueue_pop(vq, elem, in_num, out_num);
    }
}

#define TRACE_VIRTIO_QUEUE_NOTIFY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_queue_notify(void * vdev, int n, void * vq)
{
}

static inline void trace_virtio_queue_notify(void * vdev, int n, void * vq)
{
    if (true) {
        _nocheck__trace_virtio_queue_notify(vdev, n, vq);
    }
}

#define TRACE_VIRTIO_NOTIFY_IRQFD_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_notify_irqfd(void * vdev, void * vq)
{
}

static inline void trace_virtio_notify_irqfd(void * vdev, void * vq)
{
    if (true) {
        _nocheck__trace_virtio_notify_irqfd(vdev, vq);
    }
}

#define TRACE_VIRTIO_NOTIFY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_notify(void * vdev, void * vq)
{
}

static inline void trace_virtio_notify(void * vdev, void * vq)
{
    if (true) {
        _nocheck__trace_virtio_notify(vdev, vq);
    }
}

#define TRACE_VIRTIO_SET_STATUS_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_set_status(void * vdev, uint8_t val)
{
}

static inline void trace_virtio_set_status(void * vdev, uint8_t val)
{
    if (true) {
        _nocheck__trace_virtio_set_status(vdev, val);
    }
}

#define TRACE_VIRTIO_RNG_GUEST_NOT_READY_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_guest_not_ready(void * rng)
{
}

static inline void trace_virtio_rng_guest_not_ready(void * rng)
{
    if (true) {
        _nocheck__trace_virtio_rng_guest_not_ready(rng);
    }
}

#define TRACE_VIRTIO_RNG_CPU_IS_STOPPED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_cpu_is_stopped(void * rng, int size)
{
}

static inline void trace_virtio_rng_cpu_is_stopped(void * rng, int size)
{
    if (true) {
        _nocheck__trace_virtio_rng_cpu_is_stopped(rng, size);
    }
}

#define TRACE_VIRTIO_RNG_POPPED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_popped(void * rng)
{
}

static inline void trace_virtio_rng_popped(void * rng)
{
    if (true) {
        _nocheck__trace_virtio_rng_popped(rng);
    }
}

#define TRACE_VIRTIO_RNG_PUSHED_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_pushed(void * rng, size_t len)
{
}

static inline void trace_virtio_rng_pushed(void * rng, size_t len)
{
    if (true) {
        _nocheck__trace_virtio_rng_pushed(rng, len);
    }
}

#define TRACE_VIRTIO_RNG_REQUEST_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_request(void * rng, size_t size, unsigned quota)
{
}

static inline void trace_virtio_rng_request(void * rng, size_t size, unsigned quota)
{
    if (true) {
        _nocheck__trace_virtio_rng_request(rng, size, quota);
    }
}

#define TRACE_VIRTIO_RNG_VM_STATE_CHANGE_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_rng_vm_state_change(void * rng, int running, int state)
{
}

static inline void trace_virtio_rng_vm_state_change(void * rng, int running, int state)
{
    if (true) {
        _nocheck__trace_virtio_rng_vm_state_change(rng, running, state);
    }
}

#define TRACE_VIRTIO_BALLOON_BAD_ADDR_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_balloon_bad_addr(uint64_t gpa)
{
}

static inline void trace_virtio_balloon_bad_addr(uint64_t gpa)
{
    if (true) {
        _nocheck__trace_virtio_balloon_bad_addr(gpa);
    }
}

#define TRACE_VIRTIO_BALLOON_HANDLE_OUTPUT_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_balloon_handle_output(const char * name, uint64_t gpa)
{
}

static inline void trace_virtio_balloon_handle_output(const char * name, uint64_t gpa)
{
    if (true) {
        _nocheck__trace_virtio_balloon_handle_output(name, gpa);
    }
}

#define TRACE_VIRTIO_BALLOON_GET_CONFIG_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_balloon_get_config(uint32_t num_pages, uint32_t actual)
{
}

static inline void trace_virtio_balloon_get_config(uint32_t num_pages, uint32_t actual)
{
    if (true) {
        _nocheck__trace_virtio_balloon_get_config(num_pages, actual);
    }
}

#define TRACE_VIRTIO_BALLOON_SET_CONFIG_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_balloon_set_config(uint32_t actual, uint32_t oldactual)
{
}

static inline void trace_virtio_balloon_set_config(uint32_t actual, uint32_t oldactual)
{
    if (true) {
        _nocheck__trace_virtio_balloon_set_config(actual, oldactual);
    }
}

#define TRACE_VIRTIO_BALLOON_TO_TARGET_BACKEND_DSTATE() ( \
    false)

static inline void _nocheck__trace_virtio_balloon_to_target(uint64_t target, uint32_t num_pages)
{
}

static inline void trace_virtio_balloon_to_target(uint64_t target, uint32_t num_pages)
{
    if (true) {
        _nocheck__trace_virtio_balloon_to_target(target, num_pages);
    }
}
#endif /* TRACE_HW_VIRTIO_GENERATED_TRACERS_H */