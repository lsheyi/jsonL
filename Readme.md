# JsonL - tiny json library for C++11

## 期望功能

- 主功能 
    - 提供JSON解析、序列化

- 其他API 
    - 返回类型、类型判断、返回数值、支持比较、支持json文件解析、将Json对象序列化至文件

## 设计
Pimpl技巧、模板类型推导

### 核心对象 JsonL::Json 

- 成员变量 
    shared_ptr<JsonValue> m_ptr 

- 类型 
    NUL, BOOL, NUMBER, STRING, ARRAY, OBJECT

- 成员函数
    构造函数、类型判断、数值获取、运算符重载、解析、序列化


### 具体实现类

- 继承方式 
    JsonValue <-- Value <-- JsonNull, JsonBoolean, JsonInt, JsonDouble, JsonString, JsonArray, JsonObject


### 解析类 JsonParser

- 成员变量
    解析策略（是否有注释）、字符串、当前索引、失败标识、错误信息   

- 成员函数
    解析各个类型函数
    辅助函数————略过空白、略过注释等


### 序列化 静态函数

- 将Json对象按类型序列化
     提供Json类和JsonValue封装接口

## 测试
- benchmark
    google/benchmark
    - parse
    - dump

## 优化

### 向量化优化 (SIMD)
    序列化优化
    解析优化

### 按需解析

### DOM设计优化
    string_view
    内存池