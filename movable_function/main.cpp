#include <iostream>
#include <functional>
#include <memory>
#include <list>

// Без этого объявления мы не можем объявить struct Function<R(Args...)> 
// Так как R(Args...) это шаблонная специализация типа
template<typename T>
struct Function; 

// Теперь мы можем специализировать Function
// Каким-нибудь конктретным типом, например,
// template<>
// struct Function<int(double, double)> { };
//
// int(double, double) - это конкретный тип

template<typename R, typename ...Args>
struct Function<R(Args...)> {

  // Так как хотим принимать в конструкторе лямбды, а лямбды имеют разные типы,
  // Мы вынуждены создавать шаблонный конструктор от любого типа Functor.
  // Нам удобно использовать forwarding references, для идеальной передачи
  // Так как мы хотим принимать тип по значению, lvalue/rvalue ссылкам const/non-const
  // И поведение функции почти не меняется в зависимости от типа Functor
  // Для forwarding ссылок необходимо иметь выводимый из шаблонного параметра тип аргумента, и &&
  template<typename Functor>
  Function(Functor&& f) {
    // Для внутреннего класса хотим хранить по значению
    // чтобы класс оставался валидным, даже когда уничтожиться f
    using HolderValue = std::remove_reference_t<Functor>;
    callable_ = std::make_unique<Holder<HolderValue>>(std::forward<Functor>(f));
  }

  // Args совпадает с типом, указанным при объявлении Function
  // Например, при объявлении Function<void(const std::string& v, int&& i)>
  // можно будет вызывать Function только с такими параметрами
  // вызов, например, int i = 10; f("123", i); даст ошибку - и это то, что нам нужно
  void operator()(Args ...arg) {
    // Нам все равно нужен std::forward, чтобы правильно скастить arg, в случае если он rvalue
    return callable_->run(std::forward<Args>(arg)...);
  }

private:
  
  // Полиморфизм позволяет нам вызывать одинаковые
  // по сигнатуре методы у разных типов
  struct ICallable {
    virtual R run(Args ...arg) = 0;
    // Мы может не объявлять виртуальный деструктор
    // Так как наследники не будут его переопределять
  };

  // Класс, который будет держать любой вызываемый тип по значению
  // Прием называется Type Erasure - позволяет хранить разные типы как один
  template<typename T>
  class Holder : public ICallable {
  public:
    // T имеет && но не выводится из аргументов - это не forwarding ссылка 
    // Это RValue ссылка, так как мы знаем, что это rvalue - мы сами вызываем move
    Holder(T&& t) : t_{std::move(t)} {}

    // Сами определяем конструктор копирования
    Holder(const T& t) : t_{t} {}

    // Мы могли бы сделать через forwarding ссылки
    // template<typename G>
    // Holder(G&& g) : t_{std::forward<G>(g)} {} 
    // Но тогда мы даем возможность вызывать конструктор с G != T
    // Можно защитить через SFINAE, но это будет менее читабельно,
    // чем конструктор копирования и перемещения

    // args не forwarding ссылка, так как тип Args не имеет &&
    // но нам все равно нужен forward - мы не знаем rvalue это или lvalue
    R run(Args ...args) override {
      return t_(std::forward<Args>(args)...);
    }

  private:
    T t_;
  };

  std::unique_ptr<ICallable> callable_;
};

void global_func();

int main(int argc, char** argv) {
  auto shared_ptr = std::make_shared<std::string>("Hello World");
  auto unique_ptr = std::make_unique<std::string>("Hello World");
  
  auto copyable_lambda = [str=shared_ptr]() { std::cout << "Hello from copyable" << std::endl; };
  auto movable_lambda = [str=move(unique_ptr)]() { std::cout << "Hello from movable" << std::endl; };

  {
    Function<void()> f(copyable_lambda);
    f();
  }
  {
    Function<void()> f(std::move(movable_lambda));
    f();
  }

  auto rvalue_argument = [](std::string&& str) { std::cout << str << std::endl; };
  auto lvalue_argument = [](std::string& str) { str = "L value"; };
  auto value_argument = [](std::string str) { str = "Value one"; std::cout << str << std::endl; };

  {
    Function<void(std::string&&)> f(rvalue_argument);
    f("R value");
  }
  {
    Function<void(std::string&)> f(lvalue_argument);
    std::string s;
    f(s);
    std::cout << s << std::endl;
  }

  {
    Function<void(std::string)> f(value_argument);
    std::string s = "Value two";
    f(s);
    std::cout << s << std::endl;
  }

  {
    std::function<void()> f{[](){ std::cout << "Works with std::function" << std::endl; }};
    Function<void()> function = f;
    function();
  }

  {
    struct {
      void operator()() const {
        std::cout << "Works with functors" << std::endl;
      }
    } object;
    Function<void()> f = object;
    f();
  }

  {
    struct {
      void invoke() const {
        std::cout << "Works with std::bind" << std::endl;
      }
    } invoker;
    Function<void()> f = std::bind(&decltype(invoker)::invoke, invoker);
    f();
  }

  {
    Function<void()> f = &global_func;
    f();
  }
  return 0;
}

void global_func() {
  std::cout << "Works with functions" << std::endl;
}
