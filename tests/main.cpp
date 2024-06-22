//
// Created by sajith.nandasena on 18.06.2024.
//

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <concepts>
#include <iostream>

template<typename T>
concept MyConcept = std::integral<T>;

template<MyConcept T>
class Bar
{
    T t_;

public:
    explicit Bar(T t): t_{t}
    {
    }

    void print()
    {
        std::cout << "Type value is: " << t_;
    }
};


template<typename T> requires std::integral<T>
class Foo
{
    T t_;

public:
    explicit Foo(T t): t_{t}
    {
    }

    void print()
    {
        std::cout << "Type value is: " << t_ << '\n';
    }
};


void TEST_CONCEPTS()
{
    Foo<char> f{'c'};
    f.print();

    Bar<int> b{1000};
    b.print();
}

template<typename T>
void printDecayed(T &&t)
{
    using DecayedType = std::decay_t<T>;
    std::cout << "Raw type: " << typeid(T).name() << std::endl;
    std::cout << "Decayed type: " << typeid(DecayedType).name() << std::endl;
}


void TEST_DECAYED()
{
    int a = 5;
    const int &b = a;
    int c[10];

    std::string s;
    char str[100];

    printDecayed(a);
    printDecayed(b);
    printDecayed(c);
    printDecayed(s);
    printDecayed(str);

    Foo<char> foo{'c'};
    printDecayed(foo);

    Bar<int> bar{1000};
    printDecayed(bar);
}

TEST(DummyTest, test1)
{
    TEST_DECAYED();
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
