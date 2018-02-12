#include <iostream>

using namespace std;

class base
{
public:
    void p()
    {
        cout << "base 11!" << endl;
    }
};

template<class t>
class pp
{
public:
    void re()
    {
        t * tm = new t();
        tm->p();
    }

    virtual void tpp( t * pm ) = 0;
};

class ppp : public pp<base>
{
public:
    void tpp(base * pm)
    {
        re();
    }
};

class ply
{
public:
    virtual void ppy() = 0;
};

class hd
{
  public:
    virtual void dl() = 0;
};

template<class C, class Func>
class singleton : public ply
{
public:
    virtual ~singleton()
    {
        cout << "free singleton!" << endl;
    }

    template<typename ... Args>
    static C * getInstance(Args ... agrs)
    {
        if( m_instance == NULL )
        {
            m_instance = new C(agrs...);
        }

        return m_instance;
    }

    static void registerV(hd * a)
    {
        m_instance->singletonFunc().registerV(a);
    }

    Func & singletonFunc()
    {
        return ff;
    }

    singleton()
    {

    }

    virtual void ppy()
    {
        (ff)(23);
    }

private:
    static C * m_instance;
    Func ff;
};

template<class C, class Func> C * singleton<C,Func>::m_instance = NULL;

class ttFunc
{
public:
    char * operator()(int tt)
    {
       cout << "ttFunc!" << tt << endl;

    }

    void registerV(hd * a)
    {
        a->dl();
    }
};

class singletonTest : public singleton<singletonTest, ttFunc>
{
public:
    singletonTest(int a, int b)
    {
        cout << "new singletonTest!" << a << b<< endl;
    }

    virtual void pt()
    {
        cout << "hht hht singletonTest!" << endl;
    }
public:
    int a;

};

class hht : public hd
{
public:
    virtual void dl()
    {
        singletonTest * t = singletonTest::getInstance(4, 54);
        t->pt();
    }
};

template<class tt>
class tett
{
public:
    void ttt()
    {
        tt * a = NULL;
        cout <<sizeof(a)<<endl;
    }
};

int main()
{
    cout << "Hello world!" << endl;
    singletonTest * t = singletonTest::getInstance(4, 54);
    hht * tt = new hht();
    singletonTest::registerV((hd *) tt);
    ply * p = (ply * ) t;
    p->ppy();
    tett<int> * te = new tett<int> ();
    te->ttt();
    delete t;
    return 0;
}
