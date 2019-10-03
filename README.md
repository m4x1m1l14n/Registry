[![Build status](https://ci.appveyor.com/api/projects/status/3nvam4ye0yhtt5kt?svg=true)](https://ci.appveyor.com/project/m4x1m1l14n/registry)
[![Coverage Status](https://coveralls.io/repos/github/m4x1m1l14n/Registry/badge.svg?branch=master)](https://coveralls.io/github/m4x1m1l14n/Registry?branch=master)

Saved a little time of your life? :beer::sunglasses::thumbsup: 
[![paypal](https://www.paypalobjects.com/en_US/i/btn/btn_donate_SM.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=WZ5UGW93SUEA8&currency_code=EUR&amount=5&source=url)

# Registry namespace

C++ class for manipulating Windows Registry.
Wraps & simplifies native Windows API and combines it with power of modern C++11.

[//]: # "Source code documentation to whole namespace can be found on [http://www.nanosoft.sk](http://www.nanosoft.sk)"

## Table of contents
* [Table of contents](#table-of-contents)
* [Root registry keys](#root-registry-keys)
* [Opening registry keys](#opening-registry-keys)
* [Creating registry keys](#creating-registry-keys)
* [Deleting registry key](#deleting-registry-key)
* [Flush registry](#flush-registry)
* [Save registry key to file](#save-registry-key-to-file)
* [Check if registry key exists](#check-if-registry-key-exists)
* [Check if registry value exists](#check-if-registry-value-exists)
* [Save boolean value to registry](#save-boolean-value-to-registry)
* [Read boolean value from registry](#read-boolean-value-from-registry)
* [Save integer value to registry](#save-integer-value-to-registry)
* [Read integer value from registry](#read-integer-value-from-registry)
* [Save string value to registry](#save-string-value-to-registry)
* [Read string value from registry](#read-string-value-from-registry)
* [Enumerating registry subkeys](#enumerating-registry-subkeys)

>**NOTE:**  
> All methods can throw exceptions, if system error occurs!
>
> All exceptions thrown are derived from std::exception class, so don't forget to wrap your code with try / catch block.

## Root registry keys

Following windows registry root keys are predefined within Registry namespace, and can be accessed directly in your code

```C++
Registry::ClassesRoot
Registry::CurrentUser
Registry::LocalMachine
Registry::Users
Registry::CurrentConfig
```

## Opening registry keys

Existing windows registry key can be opened simply by call to RegistryKey\:\:Open() method specifying path to registry key.

>**NOTE:**  
> Registry key is closed automatically when RegistryKey object goes out of scope.

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication");

        // do work needed
    }
    catch (const std::exception&)
    {
    	// handle thrown exception
    }

    return 0;
}
```

Registry key is by default opened with read access, same as if you open registry key as follows

```C++
auto access = Registry::DesiredAccess::Read;

auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication", access);
```

Desired access for opening registry keys can be combined

```C++
auto access = Registry::DesiredAccess::Read | Registry::DesiredAccess::Write;
		
auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication", access);
```

## Creating registry keys

Registry keys can be created in a same way as if they are opened.

Simply by using RegistryKey::Create() method. Usage is same as when opening keys.

> **NOTE:**  
> You must have sufficient privileges to create registry keys under specific root key.
> In other case std::system_error exception will be thrown

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto access = Registry::DesiredAccess::Read | Registry::DesiredAccess::Write;

        auto key = Registry::LocalMachine->Create(L"SOFTWARE\\MyCompany\\MyApplication", access);

        // do work needed
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

RegistryKey can be created also from native windows registry key handle HKEY.

> **NOTE:**  
> In case hKey is nullptr, std::invalid_argument is thrown

```C++
Registry::RegistryKey_ptr key = std::make_shared<Registry::RegistryKey>(hKey);
```

## Using with native API

RegistryKey has HKEY() operator overloaded, thus can be used with conjunction with native Windows registry API.

Usage is as follows

```C++
auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication");

DWORD dwValue = 0;
DWORD cbData = sizeof(dwValue);
DWORD dwType;

LSTATUS lStatus = RegQueryValueEx(*key, L"SomeDWORDValue", nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &cbData);
if (lStatus == ERROR_SUCCESS)
{
    // DO SOMETHING
}
```

## Deleting registry key

## Flush registry

## Save registry key to file

## Check if registry key exists

HasKey() or Exists() methods can be used to check, whether registry key contains specified subkey.

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Create(L"SOFTWARE\\MyCompany\\MyApplication");
        if (key->HasKey(L"SomeSubKey"))
        {
            // TRUE
        }
        else
        {
            // FALSE
        }
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Check if registry value exists

To check if windows registry key contains specific value use HasValue() method as follows

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Create(L"SOFTWARE\\MyCompany\\MyApplication");
        if (key->HasValue(L"Version"))
        {
            // TRUE
        }
        else
        {
            // FALSE
        }
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Save boolean value to registry

To save boolean value to registry, use SetBoolean() method as follows

> **NOTE:**  
> Boolean value is stored in registry as REG_DWORD. 0 for false and 1 for true.

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Create(L"SOFTWARE\\MyCompany\\MyApplication");
		
        key->SetBoolean(L"EnableLogger", true);
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Read boolean value from registry

To read boolean value from registry use GetBoolean() method as follows

> **NOTE:**  
> RegistryKey class use REG_DWORD registry value data type to work with boolean value.
> If registry value is 0, false is returned. If registry value is above 0, value of true is returned.

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Create(L"SOFTWARE\\MyCompany\\MyApplication");
        auto enabled = key->GetBoolean(L"EnableLogger");
        if (enabled)
        {
            // do work
        }
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Save integer value to registry

To save signed / unsigned integer values to registry, use one of methods 

* SetInt32()
* SetUInt32()
* SetInt64()
* SetUInt64()

as follows

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto access = Registry::DesiredAccess::Write;

        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication\\Logger", access);
		
        key->SetInt32(L"Severity", -3);
        key->SetUInt32(L"Timeout", 10000);

        key->SetInt64(L"SomeBigNumber", -9223372036854775808);
        key->SetUInt64(L"Id", 0xf0f0f0f0f0f0f0f0);
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Read integer value from registry

To read signed / unsigned integer values from registry, use one of methods

* GetInt32()
* GetUInt32()
* GetInt64()
* GetUInt64()

as follows

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication\\Logger");
		
        std::cout << key->GetInt32(L"Severity")         << std::endl;
        std::cout << key->GetUInt32(L"Timeout")         << std::endl;
        std::cout << key->GetInt64(L"SomeBigNumber")    << std::endl;
        std::cout << key->GetUInt64(L"Id")              << std::endl;
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Save string value to registry

To save string value to registry use SetString() method as follows

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto access = Registry::DesiredAccess::Write;

        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication\\Logger", access);
		
        key->SetString(L"FileName", L"c:\\Program Files\\MyCompany\\MyApplication\\log.txt");
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Read string value from registry

To read string value from windows registry use GetString() method as follows

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication\\Logger");
		
        auto logFileName = key->GetString(L"FileName");

        // work with read value
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Save expandable string value to registry

To save string value to registry use SetExpandString() method in same manner as SetString().


```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto access = Registry::DesiredAccess::Write;

        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\MyCompany\\MyApplication\\Logger", access);
		
        key->SetExpandString(L"InstallDir", L"%ProgramFiles%\\My Company\\My Product\\");
    }
    catch (const std::exception&)
    {
        // handle thrown exception
    }

    return 0;
}
```

## Enumerating registry subkeys

Eumerating registry key subkeys is done as follows, using lambda expression

```C++
#include <Registry.hpp>

using namespace m4x1m1l14n;

int main()
{
    try
    {
        auto key = Registry::LocalMachine->Open(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");

        key->EnumerateSubKeys([](const std::wstring& name) -> bool
        {
            // Process subkey
            std::wcout << name << std::endl;

            // Return true to continue processing, false otherwise
            return true;
        });
    }
    catch (const std::exception& ex)
    {
        // handle thrown exception
    }

    return 0;
}
```
