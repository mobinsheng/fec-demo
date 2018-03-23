#ifndef MARSHALL_H
#define MARSHALL_H

#include <vector>
#include <assert.h>

using namespace std;

/*
 * 1、判断是否为小端（网络字节是大端）
 * 2、如果是那么就转换
 * 3、把数据插入buffer中
 */
#define __WRITE__(v) \
{\
    char* data = (char*)&v;\
    if(is_little_endian){\
        swap_word(data,sizeof(v));\
    }\
    net_data.insert(net_data.end(),data,data + sizeof(v));\
}

/*
 * 1、取出指定类型的数据（type：int32、int64等）
 * 2、判断是否为小端（网络字节是大端）
 * 3、如果是小端那么就转换
 * 4、返回数据
 */
#define __READ__(type)\
{\
    assert(net_data.size() >= sizeof(type));\
    type v = *((type*)&net_data[0]);\
    if(is_little_endian)\
    {\
        swap_word((char*)&v,sizeof(v));\
    }\
    pop(sizeof(v));\
    return v;\
}

#define __SAFE_CHECK__(size) assert(net_data.size() >= (size))


class Marshallable{
public:
    Marshallable(){
        is_little_endian = IsLittleEndian();
        net_data.clear();

    }

    Marshallable(const char* data,size_t len){
        is_little_endian = IsLittleEndian();
        net_data.clear();

        write(data,len);
    }

    Marshallable(const vector<char>& v){
        is_little_endian = IsLittleEndian();
        net_data.clear();

        if(!v.empty()){
            write(&v[0],v.size());
        }
    }

    virtual ~Marshallable(){
    }

    // 序列化
    virtual void marshal(){

    }

    // 反序列化
    virtual void unmarshal(){

    }

    // 清除数据
    void clear(){
        net_data.clear();
    }

    // 返回序列化之后的数据长度
    size_t get_size(){
        return net_data.size();
    }

    // 返回序列化之后的数据
    const char* get_data(){
        if(net_data.empty()){
            return NULL;
        }
        return &net_data[0];
    }

    /*********************************************
     * 写入数据/序列化
     ********************************************/
    void write_uint8(uint8_t v){
        net_data.push_back(v);
    }

    void write_int8(int8_t v){
        net_data.push_back(v);
    }


    void write_uint16(uint16_t v){
        __WRITE__(v);
    }

    void write_int16(int16_t v){
        __WRITE__(v);
    }

    void write_uint32(uint32_t v){
        __WRITE__(v);
    }

    void write_int32(int32_t v){
        __WRITE__(v);
    }

    void write_uint64(uint64_t v){
        __WRITE__(v);
    }

    void write_int64(int64_t v){
        __WRITE__(v);
    }

    // 写入普通的二进制数据
    void write(const char* v,size_t size){
        net_data.insert(net_data.end(),v,v + size);
    }

    /*********************************************
     * 读取数据/反序列化
     ********************************************/
    uint8_t read_uint8(){
        /*uint8_t v = *((uint8_t*)(&net_data[0]));
        pop(sizeof(v));
        return v;*/
        __READ__(uint8_t);
    }

    int8_t read_int8(){
        __READ__(int8_t);
    }

    uint16_t read_uint16(){
        __READ__(uint16_t);
    }

    int16_t read_int16(){
        __READ__(int16_t);
    }

    uint32_t read_uint32(){
        __READ__(uint32_t);
    }

    int32_t read_int32(){
        __READ__(int32_t);
    }

    uint64_t read_uint64(){
        __READ__(uint64_t);
    }

    int64_t read_int64(){
        __READ__(int64_t);
    }

    size_t read(char* buf,size_t len){
        assert(net_data.size() >= len);
        memmove(buf,&net_data[0],len);
        pop(len);
        return len;
    }


private:
    void pop(size_t size){
        vector<char>::iterator it = net_data.begin() + size;
        net_data.erase(net_data.begin(),it);
    }

    static void swap_word(char *binary, int len)
    {
        char tmp;
        int i, j;
        for(i = 0, j = len - 1; i < j; ++i, --j)
        {
            tmp = binary[i];
            binary[i] = binary[j];
            binary[j] = tmp;
        }
    }

    static int IsLittleEndian()
    {
        int i = 1;
        return (*(unsigned char *)&i == 1);
    }

    vector<char> net_data; // 存放序列化之后的数据
    bool is_little_endian; // 字节顺序，是否为小端
};

Marshallable& operator << (Marshallable& output,uint8_t& c);
Marshallable& operator << (Marshallable& output,int8_t& c);

Marshallable& operator << (Marshallable& output,uint16_t& c);
Marshallable& operator << (Marshallable& output,int16_t& c);

Marshallable& operator << (Marshallable& output,uint32_t& c);
Marshallable& operator << (Marshallable& output,int32_t& c);

Marshallable& operator << (Marshallable& output,uint64_t& c);
Marshallable& operator << (Marshallable& output,int64_t& c);



Marshallable& operator >> (Marshallable& input,uint8_t& c);
Marshallable& operator >> (Marshallable& input,int8_t& c);

Marshallable& operator >> (Marshallable& input,uint16_t& c);
Marshallable& operator >> (Marshallable& input,int16_t& c);

Marshallable& operator >> (Marshallable& input,uint32_t& c);
Marshallable& operator >> (Marshallable& input,int32_t& c);

Marshallable& operator >> (Marshallable& input,uint64_t& c);
Marshallable& operator >> (Marshallable& input,int64_t& c);

#endif // MARSHALL_H
