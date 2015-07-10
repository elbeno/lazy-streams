#define TESTINATOR_MAIN
#include <testinator.h>

#include <cstddef>
#include <functional>
#include <vector>

using namespace std;

//------------------------------------------------------------------------------
template <typename T>
T succ(const T&);

template <>
int succ<int>(const int& n)
{
  return n+1;
}

//------------------------------------------------------------------------------
template <typename T>
class LazyStream
{
  typedef T Head;
  typedef LazyStream<T> Tail;
  typedef function<Tail ()> Generator;

  LazyStream() {}

public:
  explicit LazyStream(const T& head)
    : m_head(head)
    , m_generator([=] () -> Tail { return Tail(succ<T>(head)); })
  {
  }

  LazyStream(const T& head, const Generator& generator)
    : m_head(head)
    , m_generator(generator)
  {
  }

  const Head& head() const { return m_head; }
  Tail tail() const { return m_generator(); }

  Tail& next()
  {
    *this = tail();
    return *this;
  }

  bool empty() const { return m_generator == 0; }

  LazyStream<T> take(size_t n) const
  {
    if (n == 0)
      return LazyStream<T>();

    LazyStream<T> s;
    s.m_head = m_head;

    Generator g = m_generator;
    s.m_generator = [=] () -> Tail { return g().take(n - 1); };

    return s;
  }

  typedef function<bool (const T&)> Predicate;
  LazyStream<T> takeWhile(Predicate p) const
  {
    if (!p(m_head))
      return LazyStream<T>();

    LazyStream<T> s;
    s.m_head = m_head;

    Generator g = m_generator;
    s.m_generator = [=] () -> Tail { return g().takeWhile(p); };

    return s;
  }

  template <typename U>
  U toContainer() const
  {
    U container;

    LazyStream<T> s = *this;
    while (!s.empty())
    {
      container.push_back(s.head());
      s.next();
    }

    return container;
  }

  template <typename U>
  LazyStream<U> map(function<U (const T&)> f) const
  {
    LazyStream<U> s;
    s.m_head = f(head());

    Generator g = m_generator;
    s.m_generator = [=] () -> Tail { return g().map(f); };

    return s;
  }

  template <typename U>
  U fold(function<U (const U&, const T&)> f, const U& seed) const
  {
    U result = seed;

    LazyStream<T> s = *this;
    while (!s.empty())
    {
      result = f(result, s.head());
      s.next();
    }
    return result;
  }

  LazyStream<T> filter(Predicate p) const
  {
    LazyStream<T> s = *this;

    while (!s.empty() && !p(s.head()))
    {
      s.next();
    }

    if (!s.empty())
    {
      Generator g = s.m_generator;
      s.m_generator = [=] () -> Tail { return g().filter(p); };
    }

    return s;
  }

private:
  Head m_head;
  Generator m_generator;
};

//------------------------------------------------------------------------------
DEF_TEST(SimpleLazyStream, Stream)
{
  LazyStream<int> s(1);
  EXPECT(s.head() == 1);
  EXPECT(s.tail().head() == 2);
  EXPECT(s.tail().tail().head() == 3);
  EXPECT(s.tail().tail().tail().head() == 4);
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(Pump, Stream)
{
  LazyStream<int> s(1);
  EXPECT(s.head() == 1);
  for (int i = 2; i < 10; ++i)
  {
    EXPECT(s.next().head() == i);
  }
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(Take, Stream)
{
  LazyStream<int> s(1);
  LazyStream<int> first5 = s.take(5);
  for (int i = 1; i <= 5; ++i)
  {
    EXPECT(first5.head() == i);
    first5.next();
  }
  return first5.empty();
}

//------------------------------------------------------------------------------
DEF_TEST(TakeWhile, Stream)
{
  LazyStream<int> s(1);
  LazyStream<int> first5 = s.takeWhile([] (int n) -> bool { return n <= 5; });
  for (int i = 1; i <= 5; ++i)
  {
    EXPECT(first5.head() == i);
    first5.next();
  }
  return first5.empty();
}

//------------------------------------------------------------------------------
DEF_TEST(ToVector, Stream)
{
  LazyStream<int> s(1);
  LazyStream<int> first5 = s.take(5);
  vector<int> v = first5.toContainer<vector<int>>();
  EXPECT(v.size() == vector<int>::size_type{5});
  for (vector<int>::size_type i = 0; i < 5; ++i)
  {
    EXPECT(v[i] == static_cast<int>(i+1));
  }
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(Map, Stream)
{
  LazyStream<int> s(1);
  LazyStream<int> evens = s.map<int>([] (int n) -> int { return n * 2; });
  for (int i = 1; i <= 5; ++i)
  {
    EXPECT(evens.head() == i * 2);
    evens.next();
  }
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(Fold, Stream)
{
  LazyStream<int> s(1);
  int triangle5 = s.take(5).fold<int>([] (int x, int y) -> int { return x + y; }, 0);
  return triangle5 == 15;
}

//------------------------------------------------------------------------------
DEF_TEST(Filter, Stream)
{
  LazyStream<int> s(1);
  LazyStream<int> evens = s.filter([] (int n) -> bool { return n % 2 == 0; });

  for (int i = 1; i <= 5; ++i)
  {
    EXPECT(evens.head() == i * 2);
    evens.next();
  }
  return true;
}

//------------------------------------------------------------------------------
DEF_TEST(Primes, Stream)
{
  function<LazyStream<int> (const LazyStream<int>&)> sieve;

  sieve = [&sieve] (const LazyStream<int>& start)
    {
      int head = start.head();

      LazyStream<int> temp = start.filter([=] (int n) -> bool { return n % head != 0; });

      return LazyStream<int>(
          head,
          [&sieve, temp] () { return sieve(temp); });
    };

  LazyStream<int> naturals(2);
  LazyStream<int> primes(sieve(naturals));

  int a[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29 };
  for (int i = 0; i < 10; ++i)
  {
    EXPECT(primes.head() == a[i]);
    primes.next();
  }
  return true;
}
