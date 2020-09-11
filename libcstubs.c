//libc2.c:(.text.libc.__aeabi_errno_addr+0x2): undefined reference to `__aeabi_read_tp'
void __aeabi_read_tp(){}
//new.cpp:(.text.libcpp.__cxa_pure_virtual+0x2): undefined reference to `abort'
void abort(){}

//rowley debug detritus:
void __putchar(int ch){}


//trace for heap users:
//libc2.c:(.text.libc.free+0xa0): undefined reference to `__heap_start__'
char __heap_start__[200];
char * const __heap_end__= &__heap_start__[200];

//-nostartupfiles seems to have gotten rid of the need for these stubs.
////destructor failure stuff, but ours won't ever actually fail
//extern "C" int __aeabi_atexit(void *object, void (*destructor)(void *), void *dso_handle){
//  return int(&destructor)+int(object)+int(dso_handle);//stupid code to get rid of gratuitous warnings.
//}
/** sometimes pure virtual functions that aren't overloaded get called anyway,
  * such as from extended classes prophylactically calling the overloaded parent,
    or constructors calling their pure virtual members */
//extern "C" void __cxa_pure_virtual(){  /* upon call of pure virtual function */
//  wtf(100000); /* ignore it, but have a place for a breakpoint */
//}


//these guys get linked in when you have explicit destructors, even if those destructors are "=default".
//... that keeps on rehappening due to the warning of 'have virtual functions but not virtual destructor' inspiring the creation of pointless destructors.
//... which I guess is a potential burden- not having destructors on classes that happen to be used statically, but might be useful 'automatically' as well.
//extern "C" {
//  void __aeabi_atexit(){
//    wtf(-1);
//  }
//  void __dso_handle(){
//    wtf(-2);
//  }
//}