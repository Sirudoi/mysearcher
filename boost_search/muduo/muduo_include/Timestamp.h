#ifndef INCLUDE_MUDUO_TIMESTAMP_H
#define INCLUDE_MUDUO_TIMESTAMP_H

#include <iostream>
#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);
    static Timestamp now();
    std::string toString() const;

private:
    int64_t microSecondsSinceEpoch_;
};

#endif // INCLUDE_MUDUO_TIMESTAMP_H