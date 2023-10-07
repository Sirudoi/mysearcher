#ifndef INCLUDE_MUDUO_NOCPYABLE_H
#define INCLUDE_MUDUO_NOCPYABLE_H

/**
 * @brief 继承该类后, 派生类无法被拷贝, 因为该基类无法被拷贝
 */
class noncopyable {
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

#endif // INCLUDE_MUDUO_NOCPYABLE_H