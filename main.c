#include <stdio.h>
#include "libusb-1.0/libusb.h"
#include <unistd.h>

int sendData(libusb_device_handle *device_handle, unsigned char *data) {
    unsigned char buffer[64];
    int len = 0;
    if(libusb_control_transfer(device_handle, 0x21, 0x09, 0x0212, 1, data, 20, 2000) < 0) {
        printf("Something went wrong sendig the data\n");
        return 0;
    }
        
    usleep(1000);
    libusb_interrupt_transfer(device_handle, 0x82, buffer, sizeof(buffer), &len, 1);
    return 1;
}

int main() {
    libusb_context *ctx = NULL;
    libusb_device **devices;
    ssize_t i;
    ssize_t cnt;
    libusb_device *device;
    struct libusb_device_descriptor desc;
    uint16_t vendor_ID = 0x0, product_ID = 0x0;
    libusb_device_handle *device_handle;
    int is_kernel_detached = 1;
    unsigned char data[20] = { 0x11, 0xff, 0x0d, 0x3c, 
                               0x00, 0x01, 0x93, 0xea,
                               0xed, 0x02, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00
                             };
    unsigned char commit_data[20] = { 0x11, 0xff, 0x0c, 0x5a,
                                      0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00,
                                      0x00, 0x00, 0x00, 0x00
                                    };
    
    if(libusb_init(&ctx) < 0) return 0;
    
    cnt = libusb_get_device_list(ctx, &devices);
    
    for(i = 0; i < cnt; i++) {
        device = devices[i];
        libusb_get_device_descriptor(device, &desc);
        if(desc.idVendor == 0x46d && desc.idProduct == 0xc331) {
            printf("Found Logitech G810 Keyboard\n");
            vendor_ID = desc.idVendor;
            product_ID = desc.idProduct;
            break;
        }       
    }
    
    libusb_free_device_list(devices, 1);
        
    if(vendor_ID == 0x0 || product_ID == 0x0) {
        printf("Device not found\n");
        return 0;
    }
        
    device_handle = libusb_open_device_with_vid_pid(ctx, vendor_ID, product_ID);
        
    if(device_handle == 0) {
        printf("Device handle is false\n");
        libusb_exit(ctx);
        ctx = NULL;
        return 0;
    }
        
    if(libusb_kernel_driver_active(device_handle, 1) == 1) {
        printf("Activating kernel");
        if(libusb_detach_kernel_driver(device_handle, 1) != 0) {
            libusb_exit(ctx);
            ctx = NULL;
            return 0;
        }
          
        is_kernel_detached = 1;
    }
        
    if(libusb_claim_interface(device_handle, 1) < 0) {
        if(is_kernel_detached == 1) {
            libusb_attach_kernel_driver(device_handle, 1);
            is_kernel_detached = 0;
        }
            
        libusb_exit(ctx);
        ctx = NULL;
        return 0;
    }
        
    sendData(device_handle, data);
    sendData(device_handle, commit_data);
    
    if(libusb_release_interface(device_handle, 1) != 0) return 0;
    
    if(is_kernel_detached == 1) {
        libusb_attach_kernel_driver(device_handle, 1);
        is_kernel_detached = 0;
    }
    
    libusb_close(device_handle);
    device_handle = NULL;
    libusb_exit(ctx);
    ctx = NULL;
    
    printf("New color has been set\n");
                    
    return 1;
}
