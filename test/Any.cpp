#include <iostream>


class Any
{
    private:
        class holer
        {
            public:
                

        };



    public:
        Any(){}
        ~Any(){}

};


class Test{
    public:
        Test() {std::cout << "构造" << std::endl;}
        Test(const Test &t) {std::cout << "拷贝" << std::endl;}
        ~Test() {std::cout << "析构" << std::endl;}
};
int main()
{
    /*
    Any a;
    {
        Test t;
        a = t;
    }
    
    a = 10;
    int *pa = a.get<int>();
    std::cout << *pa << std::endl;
    a = std::string("nihao");
    std::string *ps = a.get<std::string>();
    std::cout << *ps << std::endl;
    */
    while(1) sleep(1);
    return 0;
}