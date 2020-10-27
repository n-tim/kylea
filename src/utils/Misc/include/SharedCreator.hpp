#ifndef SHARED_CREATOR_H
#define SHARED_CREATOR_H

#include <memory>

template <typename Type>
class SharedCreator
{
public:
  template<typename ...Arg>
  static std::shared_ptr<Type> create(Arg&&...arg)
  {
    struct EnableMakeShared : public Type
    {
      EnableMakeShared(Arg&&...arg) : Type(std::forward<Arg>(arg)...) {}
    };

    return std::make_shared<EnableMakeShared>(std::forward<Arg>(arg)...);
  }
};

#endif // !SHARED_CREATOR_H
