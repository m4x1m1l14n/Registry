#pragma once

#include <Windows.h>

#include <string>
#include <memory>

#include <assert.h>
#include <tchar.h>
#include <vector>

namespace Registry {
	enum Type { NUL, BOOLEAN, INT32, INT64, STRING, BINARY };

	class RegistryValue {
	public:
		virtual Registry::Type GetType() const { return Type::NUL; }

		virtual bool IsNull() const { return true; }
		virtual bool IsNumber() const { return false; }
		virtual bool IsInt32() const { return false; }
		virtual bool IsInt64() const { return false; }
		virtual bool IsString() const { return false; }
		virtual bool IsBool() const { return false; }

		virtual bool GetBoolean() const { return false; }
		virtual long GetInt32() const { return 0; }
		virtual long long GetInt64() const { return 0; }
		virtual std::wstring GetString() const { return std::wstring(); }
		virtual std::shared_ptr<BYTE> GetBinary() const { return std::shared_ptr<BYTE>(); }
	};

	template <Registry::Type tag, typename T>
	class Value : public RegistryValue {
	protected:
		explicit Value(const T& value) : m_value(value), m_type(tag) {  }
		explicit Value(T&& value) : m_value(std::move(value)), m_type(tag) { }

	public:
		Registry::Type GetType() const override { return tag; }
		virtual bool IsNull() const override { return (m_type == Registry::Type::NUL); }
		virtual bool IsNumber() const override { return  (m_type == Registry::Type::INT32 || m_type == Registry::Type::INT64); }
		virtual bool IsInt32() const override { return  (m_type == Registry::Type::INT32); }
		virtual bool IsInt64() const override { return  (m_type == Registry::Type::INT64); }
		virtual bool IsString() const override { return  (m_type == Registry::Type::STRING); }
		virtual bool IsBool() const override { return (m_type == Registry::Type::BOOLEAN); }

	protected:
		T m_value;
		Registry::Type m_type;
	};

	class RegistryBoolean final : public Value<Type::BOOLEAN, bool> {
	public:
		RegistryBoolean(bool value) : Value(value) {}

		bool GetBoolean() const override { return m_value; }
	};

	class RegistryInt32 final : public Value<Type::INT32, long> {
	public:
		RegistryInt32(long value) : Value(value) {}

		long GetInt32() const override { return m_value; }
	};

	class RegistryInt64 final : public Value<Type::INT64, long long> {
	public:
		RegistryInt64(long long value) : Value(value) {}

		long long GetInt64() const override { return m_value; }
	};

	class RegistryString final : public Value<Type::STRING, std::wstring> {
	public:
		RegistryString(const std::wstring& value) : Value(value) {}

		std::wstring GetString() const override { return m_value; }
	};

	class RegistryBinary final : public Value<Type::BINARY, std::shared_ptr<BYTE>> {
	public:
		RegistryBinary(const std::shared_ptr<BYTE>& value) : Value(value) {}

		std::shared_ptr<BYTE> GetBinary() const override { return m_value; }
	};

	class RegistryKey;

	typedef std::shared_ptr<RegistryKey> RegistryKey_ptr;

	class RegistryKey {
	private:
		RegistryKey() : RegistryKey(nullptr) {}

		RegistryKey(const RegistryKey& other) = delete;
		RegistryKey& operator=(RegistryKey& other) = delete;

	public:
		RegistryKey(HKEY hKey) : m_hKey(hKey) { }
		~RegistryKey() {
			if ((m_hKey != nullptr) && !(
				(m_hKey >= HKEY_CLASSES_ROOT) &&
#if (WINVER >= 0x0400)
				(m_hKey <= HKEY_CURRENT_USER_LOCAL_SETTINGS)
#else
				(m_hKey <= HKEY_PERFORMANCE_DATA)
#endif
				)) {
				RegCloseKey(m_hKey);
			}
		}

		operator HKEY() const { return m_hKey; }

		RegistryKey_ptr Open(const std::wstring& path, REGSAM samDesired = KEY_READ) {
			HKEY hKey = nullptr;

			LSTATUS lStatus = RegOpenKeyEx(m_hKey, path.c_str(), 0, samDesired, &hKey);
			if (lStatus == ERROR_SUCCESS) {
				auto registryKey = new RegistryKey(hKey);
				assert(registryKey != nullptr);

				return RegistryKey_ptr(registryKey);
			}

			return RegistryKey_ptr();
		}

		RegistryKey_ptr Create(const std::wstring& path) {
			HKEY hKey = nullptr;

			LSTATUS lStatus = RegCreateKeyEx(m_hKey, path.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &hKey, nullptr);
			if (lStatus == ERROR_SUCCESS) {
				auto registryKey = new RegistryKey(hKey);
				assert(registryKey != nullptr);

				return RegistryKey_ptr(registryKey);
			}

			return RegistryKey_ptr();
		}

		bool NotifyChange(bool watchSubtree = false, DWORD dwFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES)
		{
			LSTATUS lStatus = RegNotifyChangeKeyValue(m_hKey, watchSubtree ? TRUE : FALSE, dwFilter, NULL, FALSE);

			return (lStatus == ERROR_SUCCESS);
		}

		bool NotifyChangeAsync(HANDLE hEvent, bool watchSubtree = false, DWORD dwFilter = REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET | REG_NOTIFY_CHANGE_ATTRIBUTES)
		{
			auto ret = false;

			if (hEvent != nullptr && hEvent != INVALID_HANDLE_VALUE)
			{
				LSTATUS lStatus = RegNotifyChangeKeyValue(m_hKey, watchSubtree ? TRUE : FALSE, dwFilter, hEvent, TRUE);

				ret = (lStatus == ERROR_SUCCESS);
			}
			
			return ret;
		}

		bool Exists(const std::wstring& path) 
		{
			HKEY hKey = nullptr;

			bool exist = false;
			LSTATUS lStatus = RegOpenKeyEx(m_hKey, path.c_str(), 0, KEY_READ, &hKey);
			if (hKey != nullptr) { RegCloseKey(hKey); }

			return (lStatus == ERROR_SUCCESS);
		}

		bool HasValue(const std::wstring& subKey)
		{
			auto hasValue = false;
			DWORD dwType = 0;

			LSTATUS lStatus = RegQueryValueEx(m_hKey, subKey.c_str(), nullptr, &dwType, nullptr, nullptr);
			if (lStatus == ERROR_SUCCESS)
			{
				hasValue = true;
			}

			return hasValue;
		}

		bool GetBoolean(const std::wstring& name) {
			bool ret = false;
			DWORD dwType = 0;
			DWORD dwData = 0;
			DWORD cbData = sizeof(dwData);

			LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, &dwType, (LPBYTE)&dwData, &cbData);
			if (lStatus == ERROR_SUCCESS) {
				if (dwType == REG_DWORD) {
					ret = (dwData == 0) ? false : true;
				}
			}

			return ret;
		}

		bool SetBoolean(const std::wstring& name, bool value) {
			DWORD dwValue = value ? 1 : 0;
			DWORD cbData = sizeof(dwValue);

			LSTATUS lStatus = RegSetValueEx(m_hKey, name.c_str(), 0, REG_DWORD, (const BYTE*)&dwValue, cbData);

			return (lStatus == ERROR_SUCCESS);
		}

		long GetInt32(const std::wstring& name) {
			DWORD dwData = 0;
			DWORD cbData = sizeof(dwData);

			LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)&dwData, &cbData);

			return dwData;
		}

		bool SetInt32(const std::wstring& name, long value) {
			DWORD cbData = sizeof(value);

			LSTATUS lStatus = RegSetValueEx(m_hKey, name.c_str(), 0, REG_DWORD, (const BYTE*)&value, cbData);

			return (lStatus == ERROR_SUCCESS);
		}

		long long GetInt64(const std::wstring& name) {
			long long lData = 0;
			DWORD cbData = sizeof(lData);

			LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)&lData, &cbData);

			return lData;
		}

		bool SetInt64(const std::wstring& name, long long value) {
			DWORD cbData = sizeof(value);

			LSTATUS lStatus = RegSetValueEx(m_hKey, name.c_str(), 0, REG_QWORD, (const BYTE*)&value, cbData);

			return (lStatus == ERROR_SUCCESS);
		}

		std::wstring GetString(const std::wstring& name) {
			DWORD cbData = 0;
			DWORD dwType = 0;
			std::wstring value;

			LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, &dwType, nullptr, &cbData);
			if (lStatus == ERROR_SUCCESS && (dwType == REG_SZ || dwType == REG_EXPAND_SZ)) {
				WCHAR *data = new WCHAR[cbData/2];
				if (data != nullptr) {
					lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)data, &cbData);
					if (lStatus == ERROR_SUCCESS && cbData > 1) 
					{
						if (data[cbData/2 - 1] == L'\0') 
						{
							value = std::wstring(data);
						}
						else 
						{
							value = std::wstring(data, cbData/2);
						}
					}

					delete[] data;
				}
			}

			return value;
		}

		bool SetString(const std::wstring& name, const std::wstring& value) {
			DWORD cbData = DWORD(value.length());

			LSTATUS lStatus = RegSetValueEx(m_hKey, name.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), cbData * sizeof(WCHAR));

			return (lStatus == ERROR_SUCCESS);
		}

		bool SetExpandString(const std::wstring& name, const std::wstring& value)
		{
			DWORD cbData = DWORD(value.length());

			LSTATUS lStatus = RegSetValueEx(
				m_hKey, 
				name.c_str(), 
				0, 
				REG_EXPAND_SZ, 
				reinterpret_cast<const BYTE*>(value.c_str()), 
				cbData * sizeof(WCHAR)
			);

			return (lStatus == ERROR_SUCCESS);
		}

		std::shared_ptr<BYTE> GetBinary(const std::wstring& name, size_t len) {
			DWORD cbData = DWORD(len);
			BYTE *data = new BYTE[len];
			std::shared_ptr<BYTE> ret;

			if (data != nullptr) {
				LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, data, &cbData);
				if (lStatus == ERROR_SUCCESS) {
					ret = std::shared_ptr<BYTE>(data, std::default_delete<BYTE[]>());
				}
			}

			return ret;
		}

		bool SetBinary(const std::wstring& name, const BYTE* pData, size_t len) {
			DWORD cbData = DWORD(len);

			LSTATUS lStatus = RegSetValueEx(m_hKey, name.c_str(), 0, REG_BINARY, pData, DWORD(len));

			return (lStatus == ERROR_SUCCESS);
		}

		RegistryValue GetValue(const std::wstring& name) {
			assert(m_hKey != nullptr);
			DWORD dwType = 0;
			DWORD cbData = 0;

			RegistryValue ret;

			LSTATUS err = RegQueryValueEx(m_hKey, name.c_str(), nullptr, &dwType, nullptr, &cbData);
			if (err == ERROR_SUCCESS) {
				switch (dwType)
				{
				case REG_DWORD:
				{
					long value = 0;
					cbData = sizeof(value);

					if (RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)&value, &cbData) == ERROR_SUCCESS) {
						ret = RegistryInt32(value);
					}
				}
				break;

				case REG_QWORD:
				{
					long long value = 0;
					cbData = sizeof(value);

					if (RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)&value, &cbData) == ERROR_SUCCESS) {
						ret = RegistryInt64(value);
					}
				}
				break;

				case REG_SZ:
				{
					WCHAR *data = new WCHAR[cbData];
					if (data != nullptr) {
						LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, (LPBYTE)data, &cbData);
						if (lStatus == ERROR_SUCCESS) {
							std::wstring s(data, cbData);
							ret = RegistryString(s);
						}

						delete[] data;
					}
				}
				break;

				case REG_BINARY:
				{
					BYTE *data = new BYTE[cbData];

					if (data != nullptr) {
						LSTATUS lStatus = RegQueryValueEx(m_hKey, name.c_str(), nullptr, nullptr, data, &cbData);
						if (lStatus == ERROR_SUCCESS) {
							ret = RegistryBinary(
								std::shared_ptr<BYTE>(
									data,
									std::default_delete<BYTE[]>()
									)
							);
						}
					}
				}
				break;

				default:
					break;
				}
			}

			return ret;
		}

		bool Delete()
		{
			LSTATUS lStatus = RegDeleteTree(m_hKey, NULL);

			return (lStatus == ERROR_SUCCESS);
		}

		bool Delete(const std::wstring& name) 
		{
			LSTATUS lStatus = RegDeleteValue(m_hKey, name.c_str());
			if (lStatus != ERROR_SUCCESS)
			{
				lStatus = RegDeleteTree(m_hKey, name.c_str());
			}

			return (lStatus == ERROR_SUCCESS);
		}

		bool SetValue(const std::wstring& valueName, const RegistryValue& value) {
			switch (value.GetType())
			{
			case Registry::Type::BOOLEAN: { return this->SetBoolean(valueName, value.GetBoolean()); }
			case Registry::Type::INT32: { return this->SetInt32(valueName, value.GetInt32()); }
			case Registry::Type::INT64: { return this->SetInt64(valueName, value.GetInt64()); }
			case Registry::Type::STRING: { return this->SetString(valueName, value.GetString()); }
										 //case Registry::Type::BINARY: { return this->SetBinary(valueName, *value.GetBinary(), ); }
			default:
				return false;
				break;
			}
		}

		std::vector<std::wstring> GetSubKeys()
		{
			std::vector<std::wstring> vecRet;
			DWORD    dwSubKeys = 0;
			DWORD	dwMaxSubKeyLen = 0;
			LSTATUS lStatus = RegQueryInfoKey(m_hKey, NULL,NULL, NULL, &dwSubKeys, NULL, NULL,NULL, &dwMaxSubKeyLen,NULL,NULL,NULL);
			if (lStatus == ERROR_SUCCESS && dwSubKeys > 0)
			{
				
				for (DWORD i = 0; i < dwSubKeys; i++)
				{
					WCHAR   pwszName[255];
					DWORD dwNameLen = 255;
					lStatus = RegEnumKeyEx(m_hKey, i, pwszName, &dwNameLen,NULL,NULL,NULL,NULL);
					if (lStatus == ERROR_SUCCESS)
					{
						vecRet.push_back(std::wstring(pwszName, dwNameLen));
					}
				}
			}

			return vecRet;
		}

	private:
		HKEY m_hKey;
	};

	extern RegistryKey_ptr ClassesRoot;
	extern RegistryKey_ptr CurrentUser;
	extern RegistryKey_ptr LocalMachine;
	extern RegistryKey_ptr Users;
	extern RegistryKey_ptr CurrentConfig;
}
