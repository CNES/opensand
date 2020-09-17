# Issues & TO-DO

## Parameters

* Statically typed parameters

What we're looking for is a generic system allowing us to assign parameters to components. These parameters can have whatever type you want them to have.

The actual implementation in this library is using dynamically typed parameters when it comes to the `const T Parameter::getValue<T>() const` and `void Parameter::setValue<T>(T value)` methods (same goes for the default values). These methods are dynamically typed (even if templated) because we use `std::any` (c++17) internally, which is basically an abstraction for handling `void *` data in a safer way.

**Pros :**

* We store a `std::shared_ptr<Parameter>` vector inside of a `Component`, that's all. We can handle any type of parameters inside of the same vector.
* We can assign/get any value if we know its type :

```
myComponent->getParameter("p")->setValue<Int>(32);
auto v = myComponent->getParameter("p")->getValue<Int>();
```

***...Cons :***

* If the developer didn't template well the method he is calling, he will not be warned during the compilation but only at runtime. It means this is possible :

```
auto p = myComponent->addParameter(INT, "p", "name", "desc", "unit");
p->setValue<String>("runtime error");
```

It would be the same if we use `std::variant` (C++17) instead of `std::any`. We could use a base class like `ParameterBase` such as a  `Parameter<T>` class derives from it, but we would still need when trying to get a value from it to do something :
```
auto v = myComponent->getParameter("p")->getValue<Int>();
```
which would call something like this internally :
```
return dynamic_pointer_cast<Parameter<Int>>(m_parameters[id])->getValue()
```
and then we come back to a dynamically typed solution.

-------

A viable solution would be to store a vector for each type of parameter like
```
std::vector<std::shared_ptr<Parameter<Int>>> m_parametersInt;
std::vector<std::shared_ptr<Parameter<Float>>> m_parametersFloat;
[...]
```
but it would require more maintenance work. Each choice has advantages and disadvantages and changing the current implementation requires weighing the pros and cons.

## Nice to have

Also, some nice to have features (not exhaustive) are :

* Overload the `[]` operator to allow getting Component/List/Parameter by specifying their ID in the `[]` operator
* Add a way to handle constraints (especially **regex**) on parameter values (should be easier if integrated using the XSD)