// This file is distributed under the BSD License.
// See "license.txt" for details.
// Copyright 2009, Jonathan Turner (jturner@minnow-lang.org)
// and Jason Turner (lefticus@gmail.com)
// http://www.chaiscript.com

#include <iostream>

#include <chaiscript/chaiscript.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>
#include <boost/function.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>

void log(const std::string &msg)
{
  std::cout << "[" << boost::posix_time::microsec_clock::local_time() << "] " << msg << std::endl;
}

void log(const std::string &module, const std::string &msg)
{
  std::cout << "[" << boost::posix_time::microsec_clock::local_time() << "] <" << module << "> " << msg << std::endl;
}

struct System
{
  std::map<std::string, boost::function<std::string (const std::string &) > > m_callbacks;

  void add_callback(const std::string &t_name, 
      const chaiscript::Proxy_Function &t_func)
  {
    m_callbacks[t_name] = chaiscript::functor<std::string (const std::string &)>(t_func);
  }


  void do_callbacks(const std::string &inp)
  {
    log("Running Callbacks: " + inp);
    for (std::map<std::string, boost::function<std::string (const std::string &)> >::iterator itr = m_callbacks.begin();
         itr != m_callbacks.end();
         ++itr)
    {
      log("Callback: " + itr->first, itr->second(inp));
    }
  }
};


int main(int argc, char *argv[]) {
  using namespace chaiscript;

  ChaiScript chai;

  //Create a new system object and share it with the chaiscript engine
  System system;
  chai.add(var(&system), "system");

  //Register the two methods of the System structure.
  chai.add(fun(&System::add_callback), "add_callback");
  chai.add(fun(&System::do_callbacks), "do_callbacks");

  // Let's use chaiscript to add a new lambda callback to our system. 
  // The function "{ 'Callback1' + x }" is created in chaiscript and passed into our C++ application
  // in the "add_callback" function of struct System the chaiscript function is converted into a 
  // boost::function, so it can be handled and called easily and type-safely
  chai.eval("system.add_callback('#1', fun(x) { 'Callback1 ' + x });");
  
  // Because we are sharing the "system" object with the chaiscript engine we have equal
  // access to it both from within chaiscript and from C++ code
  system.do_callbacks("TestString");
  chai.eval("system.do_callbacks(\"TestString\");");

  // The log function is overloaded, therefore we have to give the C++ compiler a hint as to which
  // version we want to register. One way to do this is to create a typedef of the function pointer
  // then cast your function to that typedef.
  typedef void (*PlainLog)(const std::string &);
  typedef void (*ModuleLog)(const std::string &, const std::string &);
  chai.add(fun(PlainLog(&log)), "log");
  chai.add(fun(ModuleLog(&log)), "log");

  chai.eval("log('Test Message')");

  // A shortcut to using eval is just to use the chai operator()
  chai("log('Test Module', 'Test Message');");

  //Finally, it is possible to register any boost::function as a system function, in this 
  //way, we can, for instance add a bound member function to the system
  chai.add(fun(boost::function<void ()>(boost::bind(&System::do_callbacks, boost::ref(system), "Bound Test"))), "do_callbacks");

  //Call bound version of do_callbacks
  chai("do_callbacks()");

  boost::function<void ()> caller = chai.functor<void ()>("fun() { system.do_callbacks(\"From Functor\"); }");
  caller();


  //If we would like a type-safe return value from all call, we can use
  //the templated version of eval:
  int i = chai.eval<int>("5+5");

  std::cout << "5+5: " << i << std::endl;

  //Add a new variable
  chai("var scripti = 15");

  //We can even get a handle to the variables in the system
  int &scripti = chai.eval<int &>("scripti");

  std::cout << "scripti: " << scripti << std::endl;
  scripti *= 2;
  std::cout << "scripti (updated): " << scripti << std::endl;
  chai("print(\"Scripti from chai: \" + to_string(scripti))");

  // Add examples of handling Boxed_Values directly when needed

  // Add usage model for mixed use:
  // chai.eval("call(?, ?)", 5, "hello world"); or something


  //Creating a functor on the stack and using it immediatly 
  int x = chai.functor<int (int, int)>("fun (x, y) { return x + y; }")(5, 6);

  log("Functor test output", boost::lexical_cast<std::string>(x));


  //Ability to create our own container types when needed. std::vector and std::map are
  //mostly supported currently
  chai.add(vector_type<std::vector<int> >("IntVector"));

}

