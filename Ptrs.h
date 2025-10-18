#pragma once

// Unique

template<typename T>
class unique_ptr
{
public:
	unique_ptr() = default;
	unique_ptr(const unique_ptr&) = delete;
	unique_ptr(unique_ptr&& other) noexcept
		: m_Ptr(other.m_Ptr)
	{
		other.m_Ptr = nullptr;
	}
	unique_ptr(T* raw)
		: m_Ptr(raw)
	{
	}
	~unique_ptr()
	{
		delete m_Ptr;
	}
	unique_ptr& operator=(const unique_ptr&) = delete;

	T* get() { return m_Ptr; }
	const T* get() const { return m_Ptr; }

	T& operator*() { return *m_Ptr; }
	const T& operator*() const { return *m_Ptr; }

	T* operator->() { return m_Ptr; }
	const T* operator->() const { return m_Ptr; }

	operator bool() const { return m_Ptr; }
private:
	T* m_Ptr = nullptr;
};

template<typename T, typename... Args>
static unique_ptr<T> make_unique(Args&&... args)
{
	return unique_ptr<T>{ new T(std::forward<Args>(args)...) };
}

// Shared

template<typename T>
class shared_ptr
{
public:
	struct control_block_t
	{
		std::atomic<size_t> reference_count = 0;

		virtual ~control_block_t() = default;
		virtual void destroy_managed() {}
		void* fn_destroy_managed = nullptr;

		void add_ref()
		{
			reference_count++;
		}

		void drop_ref()
		{
			reference_count--;
			if (reference_count != 0)
				return;

			// destroy/delete managed object and self
			destroy_managed();
			delete this;
		}
	};

	struct inplace_control_block : public control_block_t
	{
		T managed;

		template<typename... Args>
		inplace_control_block(Args&&... args)
			: managed(std::forward<Args>(args)...)
		{
		}
	};

	struct dynamic_control_block : public control_block_t
	{
		T* managed = nullptr;

		dynamic_control_block(T* managed)
			: managed(managed)
		{
			control_block_t::add_ref();
		}

		void destroy_managed() override
		{
			delete managed;
		}
	};
public:
	shared_ptr() = default;
	shared_ptr(control_block_t* control, T* ptr)
		: m_control(control), m_ptr(ptr)
	{
		m_control->add_ref();
	}
	shared_ptr(T* ptr)
		: m_ptr(ptr)
	{
		m_control = new dynamic_control_block(ptr);
	}
	shared_ptr(const shared_ptr<T>& other)
		: m_control(other.m_control), m_ptr(other.m_ptr)
	{
		add_control_ref();
	}

	~shared_ptr()
	{
		drop_control_ref();
	}

	shared_ptr<T>& operator=(const shared_ptr<T>& other)
	{
		m_ptr = other.m_ptr;
		m_control = other.m_control;
		add_control_ref();
		return *this;
	}

	T* get() const { return m_ptr; }

	void reset(T* ptr = nullptr)
	{
		drop_control_ref();

		if (!ptr)
		{
			m_control = nullptr;
			m_ptr = nullptr;
			return;
		}

		m_ptr = ptr;
		m_control = new dynamic_control_block(ptr);
	}

	bool empty() const { return m_ptr; }
	operator bool() const { return m_ptr; }

	T& operator*() { return *m_ptr; }
	const T& operator*() const { return *m_ptr; }

	T* operator->() { return m_ptr; }
	const T* operator->() const { return m_ptr; }
private:
	void add_control_ref()
	{
		if (m_control)
			m_control->add_ref();
	}
	void drop_control_ref()
	{
		if (m_control)
			m_control->drop_ref();
	}
private:
	T* m_ptr = nullptr;
	control_block_t* m_control = nullptr;
};