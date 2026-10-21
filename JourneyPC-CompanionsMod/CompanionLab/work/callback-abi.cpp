class Callback { public: virtual void Run(void *p)=0; virtual void Run(void *p,bool fail,unsigned long long call)=0; virtual int Size()=0; protected: unsigned char flags;int id; };
class Test: public Callback { public: void Run(void *p) override {} void Run(void *p,bool fail,unsigned long long call) override {} int Size() override {return 712;} };
Test object;
