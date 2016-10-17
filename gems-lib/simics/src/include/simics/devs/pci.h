/*
 * simics/devs/pci.h
 *
 * This file is part of Virtutech Simics
 *
 * Copyright (C) 1998-2005 Virtutech AB, All Rights Reserved
 *
 * The Virtutech Simics API is distributed under license. Please refer to
 * the 'LICENSE' file for details.
 *
 * For documentation on the Virtutech Simics API, please refer to the
 * Simics Reference Manual. Please report any difficulties you encounter
 * with this API through the Virtutech Simics website at www.simics.com
 *
 */

#ifndef _SIMICS_DEVS_PCI_H
#define _SIMICS_DEVS_PCI_H

/* <add-type id="pci_device_interface_t">
   This interface is implemented by all PCI devices (including bridges).
   </add-type>
 */
typedef struct {
        void (*bus_reset)(conf_object_t *obj);

        /* Interrupt Acknowlege cycle */
        int (*interrupt_acknowledge)(conf_object_t *obj);

        /* Special Cycle */
        void (*special_cycle)(conf_object_t *obj, uint32 value);

        /* System Error */
        void (*system_error)(conf_object_t *obj);

        /* peer-to-peer interrupt mechanism */
        void (*interrupt_raised)(conf_object_t *obj, int pin);
        void (*interrupt_lowered)(conf_object_t *obj, int pin);
} pci_device_interface_t;

#define PCI_DEVICE_INTERFACE "pci-device"
/* ADD INTERFACE pci_device_interface */

/* <add-type id="pci_bus_interface_t">
   No documentation available!
   </add-type>
 */
typedef struct {
        exception_type_t (*memory_access)(conf_object_t *obj,
                                          generic_transaction_t *mem_op);
        void (*raise_interrupt)(conf_object_t *obj, conf_object_t *dev,
                                int pin);
        void (*lower_interrupt)(conf_object_t *obj, conf_object_t *dev,
                                int pin);
        int (*interrupt_acknowledge)(conf_object_t *obj);
        int (*add_map)(conf_object_t *obj, conf_object_t *dev,
                       addr_space_t space, conf_object_t *target,
                       map_info_t info);
        int (*remove_map)(conf_object_t *obj, conf_object_t *dev,
                          addr_space_t space, int function);
	void (*set_bus_number)(conf_object_t *obj, int bus_id);
	void (*set_sub_bus_number)(conf_object_t *obj, int bus_id);
        void (*add_default)(conf_object_t *obj, conf_object_t *dev,
                            addr_space_t space, conf_object_t *target,
                            map_info_t info);
        void (*remove_default)(conf_object_t *obj, addr_space_t space);
        void (*bus_reset)(conf_object_t *obj);
        void (*special_cycle)(conf_object_t *obj, uint32 value);
        void (*system_error)(conf_object_t *obj);

        int (*get_bus_address)(conf_object_t *obj, conf_object_t *dev);

        void (*set_device_timing_model)(conf_object_t *obj,
                                        conf_object_t *dev,
                                        conf_object_t *timing_model);
        // Only available since build 1318
        void (*set_device_status)(conf_object_t *obj, int device, int function,
                                  int enabled);
} pci_bus_interface_t;

#define PCI_BUS_INTERFACE		"pci-bus"

/* <add-type id="pci_bridge_interface_t">
   This interface is implemented by all PCI bridges.
   </add-type>
 */
typedef struct {
        void (*system_error)(conf_object_t *obj);
        void (*raise_interrupt)(conf_object_t *obj, conf_object_t *irq_obj,
                                int device, int pin);
        void (*lower_interrupt)(conf_object_t *obj, conf_object_t *irq_obj,
                                int device, int pin);
} pci_bridge_interface_t;

#define PCI_BRIDGE_INTERFACE		"pci-bridge"


/* <add-type id="pci_interrupt_interface_t">
   This interface should only be used when a device other than the bridge
   handles PCI interrupts on the PCI bus. The initiating device is specified
   with a PCI device number, and the pin represents PCI interrupt lines
   (A, B, C, or D) as numbers in the range of 0 to 3.
   </add-type>
 */
typedef struct {
        void (*raise_interrupt)(conf_object_t *obj, conf_object_t *irq_obj,
                                int device, int pin);
        void (*lower_interrupt)(conf_object_t *obj, conf_object_t *irq_obj,
                                int device, int pin);
} pci_interrupt_interface_t;

#define PCI_INTERRUPT_INTERFACE		"pci-interrupt"
/* ADD INTERFACE pci_interrupt_interface */

#define DEVICE_CONF_FUNC      0xff

#if defined(PCI_EXPRESS) || defined(DOC)
#define DEVICE_PCIE_CONF_FUNC 0xfe

/*
   <add-type id="pcie_message_type_t def">
   </add-type>
*/
typedef enum {
        /* INTx emulation */
        PCIE_Msg_Assert_INTA       = 0x20,
        PCIE_Msg_Assert_INTB       = 0x21,
        PCIE_Msg_Assert_INTC       = 0x22,
        PCIE_Msg_Assert_INTD       = 0x23,
        PCIE_Msg_Deassert_INTA     = 0x24,
        PCIE_Msg_Deassert_INTB     = 0x25,
        PCIE_Msg_Deassert_INTC     = 0x26,
        PCIE_Msg_Deassert_INTD     = 0x27,

        /* Power Management */
        PCIE_PM_Active_State_Nak   = 0x14,
        PCIE_PM_PME                = 0x18,
        PCIE_PM_Turn_Off           = 0x19,
        PCIE_PM_PME_TO_Ack         = 0x1B,

        /* Error Messages */
        PCIE_ERR_COR               = 0x30,
        PCIE_ERR_NONFATAL          = 0x31,
        PCIE_ERR_FATAL             = 0x33,

        /* Locked Transaction */
        PCIE_Locked_Transaction    = 0x00,

        /* Slot Power Limit */
        PCIE_Set_Slot_Power_Limit  = 0x90,

        /* Hot Plug Messages */
        PCIE_HP_Power_Indicator_On        = 0x45,
        PCIE_HP_Power_Indicator_Blink     = 0x47,
        PCIE_HP_Power_Indicator_Off       = 0x44,
        PCIE_HP_Attention_Button_Pressed  = 0x48,
        PCIE_HP_Attention_Indicator_On    = 0x41,
        PCIE_HP_Attention_Indicator_Blink = 0x43,
        PCIE_HP_Attention_Indicator_Off   = 0x40
} pcie_message_type_t;

typedef struct {
        int (*send_message)(conf_object_t *dst, conf_object_t *src,
                            pcie_message_type_t type, byte_string_t payload);
} pci_express_interface_t;

#define PCI_EXPRESS_INTERFACE "pci-express"

#endif   /* PCI_EXPRESS || DOC */

#endif /* _SIMICS_DEVS_PCI_H */
