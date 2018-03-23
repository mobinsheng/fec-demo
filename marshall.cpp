#include "marshall.h"

Marshallable& operator << (Marshallable& output,uint8_t& c){
    output.write_uint8(c);
    return output;
}

Marshallable& operator << (Marshallable& output,int8_t& c){
    output.write_int8(c);
    return output;
}

Marshallable& operator << (Marshallable& output,uint16_t& c){
    output.write_uint16(c);
    return output;
}

Marshallable& operator << (Marshallable& output,int16_t& c){
    output.write_int16(c);
    return output;
}

Marshallable& operator << (Marshallable& output,uint32_t& c){
    output.write_uint32(c);
    return output;
}

Marshallable& operator << (Marshallable& output,int32_t& c){
    output.write_int32(c);
    return output;
}

Marshallable& operator << (Marshallable& output,uint64_t& c){
    output.write_uint64(c);
    return output;
}

Marshallable& operator << (Marshallable& output,int64_t& c){
    output.write_int64(c);
    return output;
}

//==========================================
Marshallable& operator >> (Marshallable& input,uint8_t& c){
   c = input.read_uint8();
   return input;
}

Marshallable& operator >> (Marshallable& input,int8_t& c){
    c = input.read_int8();
    return input;
 }

Marshallable& operator >> (Marshallable& input,uint16_t& c){
    c = input.read_uint16();
    return input;
 }

Marshallable& operator >> (Marshallable& input,int16_t& c){
    c = input.read_int16();
    return input;
 }

Marshallable& operator >> (Marshallable& input,uint32_t& c){
    c = input.read_uint32();
    return input;
 }

Marshallable& operator >> (Marshallable& input,int32_t& c){
    c = input.read_int32();
    return input;
 }

Marshallable& operator >> (Marshallable& input,uint64_t& c){
    c = input.read_uint64();
    return input;
 }

Marshallable& operator >> (Marshallable& input,int64_t& c){
    c = input.read_int64();
    return input;
 }
