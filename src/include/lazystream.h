#pragma once

#include <cstddef>
#include <functional>

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
  typedef std::function<Tail ()> Generator;

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

  LazyStream<T> take(std::size_t n) const
  {
    if (n == 0)
      return LazyStream<T>();

    LazyStream<T> s;
    s.m_head = m_head;

    Generator g = m_generator;
    s.m_generator = [=] () -> Tail { return g().take(n - 1); };

    return s;
  }

  typedef std::function<bool (const T&)> Predicate;
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
  LazyStream<U> map(std::function<U (const T&)> f) const
  {
    LazyStream<U> s;
    s.m_head = f(head());

    Generator g = m_generator;
    s.m_generator = [=] () -> Tail { return g().map(f); };

    return s;
  }

  template <typename U>
  U fold(std::function<U (const U&, const T&)> f, const U& seed) const
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
