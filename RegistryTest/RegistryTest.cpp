#include "stdafx.h"

#include <iostream>

#include <Registry.hpp>

using namespace m4x1m1l14n;

#define STRINGIFY(expr) #expr

#define CHECK_THROWS_AS(expression, exceptionType)		\
	do {												\
		std::cout << STRINGIFY(expression) << ": ";		\
		try {											\
			static_cast<void>(expression);				\
			assert(0);									\
		}												\
		catch (const exceptionType e) {					\
			std::cout << e.what();						\
			std::cout << " OK";							\
		}												\
		catch (...) {									\
			std::cout << "FAILED";						\
			throw;										\
		}												\
		std::cout << std::endl;							\
} while (0)


#define CHECK_NO_THROW(expression)						\
	do {												\
		std::cout << STRINGIFY(expression) << ": ";		\
		try {											\
			static_cast<void>(expression);				\
			std::cout << " OK";							\
		}												\
		catch (const std::exception& e) {				\
			std::cout << e.what();						\
			throw;										\
		}												\
		catch (...) {									\
			std::cout << "FAILED";						\
			throw;										\
		}												\
		std::cout << std::endl;							\
} while (0)

std::wstring gen_random(const int len)
{
	static const wchar_t alphanum[] =
		L"0123456789"
		L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		L"abcdefghijklmnopqrstuvwxyz";

	std::wstring s;

	for (int i = 0; i < len; ++i) 
	{
		s += alphanum[rand() % (ARRAYSIZE(alphanum) - 1)];
	}

	return s;
}

int main()
{
	srand(static_cast<unsigned int>(time(nullptr)));

	// New registry key creation from nullptr should throw
	CHECK_THROWS_AS(Registry::RegistryKey(nullptr), std::invalid_argument&);

	// Opening registry keys

	// Open registry key with empty path
	CHECK_THROWS_AS(Registry::LocalMachine->Open(L""), std::invalid_argument&);
	// Open non-existing key
	CHECK_THROWS_AS(Registry::LocalMachine->Open(L"NOT_EXISTING_REGISTRY_KEY"), std::system_error&);
	// Create registry key with empty path
	CHECK_THROWS_AS(Registry::LocalMachine->Create(L""), std::invalid_argument&);
	// Cannot create registry key right in local
	CHECK_THROWS_AS(Registry::LocalMachine->Create(L"CANNOT_CREATE_ON_HIVE"), std::system_error&);

	{
		auto key = Registry::LocalMachine->Open(L"SOFTWARE");

		CHECK_THROWS_AS(key->HasKey(L""), std::invalid_argument&);
		CHECK_THROWS_AS(key->Exists(L""), std::invalid_argument&);

		assert(key->HasKey(L"KEY_THAT_DOES_NOT_EXISTS") == false);
		assert(key->Exists(L"KEY_THAT_DOES_NOT_EXISTS") == false);

		CHECK_THROWS_AS(key->Create(L"CANNOT_CREATE_ACCESS_DENIED"), std::system_error&);
	}

	{
		auto desiredAccess = Registry::DesiredAccess::AllAccess | Registry::DesiredAccess::Notify;
		auto subKey = Registry::CurrentUser->Create(L"OUR_TESTING_SUBKEY", desiredAccess);

		CHECK_NO_THROW(subKey->SetExpandString(L"%ProgramFiles%\\My Company\\My Product\\Program.exe"));
		assert(subKey->GetString() == L"%ProgramFiles%\\My Company\\My Product\\Program.exe");

		{
			auto task = concurrency::create_task([subKey]() -> std::wstring
			{
				std::wstring result;

				auto hChangeEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
				assert(hChangeEvent != nullptr);

				CHECK_NO_THROW(subKey->NotifyAsync(hChangeEvent));

				DWORD dwWaitResult = ::WaitForSingleObject(hChangeEvent, INFINITE);
				if (dwWaitResult == WAIT_OBJECT_0)
				{
					result = subKey->GetString(L"INVOKE_NOTIFY");
				}

				::CloseHandle(hChangeEvent);

				return result;
			});

			::Sleep(1000);

			const auto& val = gen_random(32);

			CHECK_NO_THROW(subKey->SetString(L"INVOKE_NOTIFY", val));
			
			assert(task.get() == val);
			CHECK_NO_THROW(subKey->Delete(L"INVOKE_NOTIFY"));
		}

		CHECK_THROWS_AS(subKey->HasValue(L""), std::invalid_argument&);
		assert(subKey->HasValue(L"VALUE_THAT_DOES_NOT_EXISTS") == false);

		// Int32
		CHECK_NO_THROW(subKey->SetInt32(L"INT32_VALUE_THAT_EXISTS", 1001236));
		assert(subKey->HasValue(L"INT32_VALUE_THAT_EXISTS") == true);
		assert(subKey->GetInt32(L"INT32_VALUE_THAT_EXISTS") == 1001236);
		CHECK_NO_THROW(subKey->Delete(L"INT32_VALUE_THAT_EXISTS"));
		assert(subKey->HasValue(L"INT32_VALUE_THAT_EXISTS") == false);
		CHECK_NO_THROW(subKey->Delete(L"INT32_VALUE_THAT_EXISTS"));

		// Int64
		CHECK_NO_THROW(subKey->SetInt64(L"INT64_VALUE_THAT_EXISTS", 810012361001236));
		assert(subKey->HasValue(L"INT64_VALUE_THAT_EXISTS") == true);
		assert(subKey->GetInt64(L"INT64_VALUE_THAT_EXISTS") == 810012361001236);
		CHECK_NO_THROW(subKey->Delete(L"INT64_VALUE_THAT_EXISTS"));
		assert(subKey->HasValue(L"INT64_VALUE_THAT_EXISTS") == false);
		CHECK_NO_THROW(subKey->Delete(L"INT64_VALUE_THAT_EXISTS"));

		// Boolean
		CHECK_NO_THROW(subKey->SetBoolean(L"BOOLEAN_VALUE_THAT_EXISTS", true));
		assert(subKey->HasValue(L"BOOLEAN_VALUE_THAT_EXISTS") == true);
		assert(subKey->GetBoolean(L"BOOLEAN_VALUE_THAT_EXISTS") == true);
		CHECK_NO_THROW(subKey->Delete(L"BOOLEAN_VALUE_THAT_EXISTS"));
		assert(subKey->HasValue(L"BOOLEAN_VALUE_THAT_EXISTS") == false);
		CHECK_NO_THROW(subKey->Delete(L"BOOLEAN_VALUE_THAT_EXISTS"));

		// String
		CHECK_NO_THROW(subKey->SetString(L"STRING_VALUE_THAT_EXISTS", L"Value I want to store in this registry key!"));
		assert(subKey->HasValue(L"STRING_VALUE_THAT_EXISTS") == true);
		assert(subKey->GetString(L"STRING_VALUE_THAT_EXISTS") == L"Value I want to store in this registry key!");
		CHECK_NO_THROW(subKey->Delete(L"STRING_VALUE_THAT_EXISTS"));
		assert(subKey->HasValue(L"STRING_VALUE_THAT_EXISTS") == false);
		CHECK_NO_THROW(subKey->Delete(L"STRING_VALUE_THAT_EXISTS"));

		// Delete named key with values
		{
			auto tmpKey = subKey->Create(L"TEST1", Registry::DesiredAccess::AllAccess);

			// Test default registry key values
			CHECK_NO_THROW(tmpKey->SetInt32(-61));					assert(tmpKey->GetInt32() == -61);
			CHECK_NO_THROW(tmpKey->SetUInt32(61));					assert(tmpKey->GetUInt32() == 61);
			CHECK_NO_THROW(tmpKey->SetInt64(61));					assert(tmpKey->GetInt64() == 61);
			CHECK_NO_THROW(tmpKey->SetUInt32(61));					assert(tmpKey->GetUInt64() == 61);
			CHECK_NO_THROW(tmpKey->SetString(L"Default"));			assert(tmpKey->GetString() == L"Default");

			// Boolean
			CHECK_NO_THROW(tmpKey->SetBoolean(L"AA", true));		assert(tmpKey->GetBoolean(L"AA") == true);
			CHECK_NO_THROW(tmpKey->SetBoolean(L"AB", false));		assert(tmpKey->GetBoolean(L"AB") == false);

			// Int32
			CHECK_NO_THROW(tmpKey->SetInt32(L"BA", 0));				assert(tmpKey->GetInt32(L"BA") == 0);
			CHECK_NO_THROW(tmpKey->SetInt32(L"BB", 1));				assert(tmpKey->GetInt32(L"BB") == 1);
			CHECK_NO_THROW(tmpKey->SetInt32(L"BC", -1));			assert(tmpKey->GetInt32(L"BC") == -1);
			CHECK_NO_THROW(tmpKey->SetInt32(L"BD", MAXINT32));		assert(tmpKey->GetInt32(L"BD") == MAXINT32);
			CHECK_NO_THROW(tmpKey->SetInt32(L"BE", MININT32));		assert(tmpKey->GetInt32(L"BE") == MININT32);

			// UInt32
			CHECK_NO_THROW(tmpKey->SetUInt32(L"CA", 0));			assert(tmpKey->GetUInt32(L"CA") == 0);
			CHECK_NO_THROW(tmpKey->SetUInt32(L"CB", 1));			assert(tmpKey->GetUInt32(L"CB") == 1);
			CHECK_NO_THROW(tmpKey->SetUInt32(L"CC", -1));			assert(tmpKey->GetUInt32(L"CC") == -1);
			CHECK_NO_THROW(tmpKey->SetUInt32(L"CD", MAXUINT32));	assert(tmpKey->GetUInt32(L"CD") == MAXUINT32);

			// Int64
			CHECK_NO_THROW(tmpKey->SetInt64(L"DA", 0));				assert(tmpKey->GetInt64(L"DA") == 0);
			CHECK_NO_THROW(tmpKey->SetInt64(L"DB", 1));				assert(tmpKey->GetInt64(L"DB") == 1);
			CHECK_NO_THROW(tmpKey->SetInt64(L"DC", -1));			assert(tmpKey->GetInt64(L"DC") == -1);
			CHECK_NO_THROW(tmpKey->SetInt64(L"DD", MAXINT64));		assert(tmpKey->GetInt64(L"DD") == MAXINT64);
			CHECK_NO_THROW(tmpKey->SetInt64(L"DE", MININT64));		assert(tmpKey->GetInt64(L"DE") == MININT64);

			// UInt64
			CHECK_NO_THROW(tmpKey->SetUInt64(L"EA", 0));			assert(tmpKey->GetUInt64(L"EA") == 0);
			CHECK_NO_THROW(tmpKey->SetUInt64(L"EB", 1));			assert(tmpKey->GetUInt64(L"EB") == 1);
			CHECK_NO_THROW(tmpKey->SetUInt64(L"EC", -1));			assert(tmpKey->GetUInt64(L"EC") == -1);
			CHECK_NO_THROW(tmpKey->SetUInt64(L"ED", MAXUINT64));	assert(tmpKey->GetUInt64(L"ED") == MAXUINT64);

			// String
			CHECK_NO_THROW(tmpKey->SetString(L"FA", L""));			assert(tmpKey->GetString(L"FA") == L"");
			CHECK_NO_THROW(tmpKey->SetString(L"FB", L"A"));			assert(tmpKey->GetString(L"FB") == L"A");

			const std::wstring val = L"jhihsihjo; ;oj9dn9u8y   8726yi7138ry301ccn   f  fjhiehfo2h 2 c2jhcoh293i70473[]\\;;lll[]]\\[;'.,.\\áýáýíwýžž+=éíáýýž;;```";
			CHECK_NO_THROW(tmpKey->SetString(L"FC", val));			assert(tmpKey->GetString(L"FC") == val);
		}

		CHECK_NO_THROW(subKey->Delete(L"TEST1"));

		CHECK_NO_THROW(subKey->Delete());
	}

	return 0;
}
