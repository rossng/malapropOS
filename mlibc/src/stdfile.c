#include <stdfile.h>

fat16_file_attr_t unpack_file_attributes(uint8_t attributes_byte) {
        fat16_file_attr_t result;
        result.is_read_only = (attributes_byte & 0x01) && 1;
        result.is_hidden_file = (attributes_byte & 0x02) && 1;
        result.is_system_file = (attributes_byte & 0x04) && 1;
        result.is_volume_label = (attributes_byte & 0x08) && 1;
        result.is_subdirectory = (attributes_byte & 0x10) && 1;
        result.is_archive = (attributes_byte & 0x20) && 1;

        return result;
}

uint8_t pack_file_attributes(fat16_file_attr_t attributes) {
        uint8_t result;
        if (attributes.is_read_only) { result |= 0x01; }
        if (attributes.is_hidden_file) { result |= 0x02; }
        if (attributes.is_system_file) { result |= 0x04; }
        if (attributes.is_volume_label) { result |= 0x08; }
        if (attributes.is_subdirectory) { result |= 0x10; }
        if (attributes.is_archive) { result |= 0x20; }

        return result;
}
