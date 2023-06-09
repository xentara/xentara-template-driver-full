// Copyright (c) embedded ocean GmbH
#pragma once

#include <xentara/utils/atomic/Optional.hpp>
#include <xentara/utils/atomic/UniquePointer.hpp>

#include <string>
#include <concepts>

namespace xentara::plugins::templateDriver
{

/// @brief A thread-safe, lock-free queue that can hold a single value.
///
/// This queues only allows enqueuing a single value. Enqueuing a second value will overwrite the first.
template <typename DataType>
class SingleValueQueue final
{
public:
	/// @brief the value type
	using value_type = DataType;

	/// @brief Enqueues a value.
	/// 
	/// Any value already in the queue will be replaced.
	/// @param value The value to place in the queue
	auto enqueue(const value_type &value) noexcept -> void
	{
		_value.store(value, std::memory_order_release);
	}

	/// @brief Gets the last scheduled value and removes it from the queue
	/// @return The scheduled value or std::nullopt if none was scheduled since the last call
	auto dequeue() noexcept -> std::optional<value_type>
	{
		return _value.exchange(std::nullopt, std::memory_order_acq_rel);
	}

private:
	/// @brief The queued value, or std::nullopt if the queue is empty.
	utils::atomic::Optional<value_type> _value;

	// Check that the value is lock free, or blocking will occurr
	static_assert(decltype(_value)::is_always_lock_free);
};

/// @brief Specialization of SingleValueQueue for std::basic_string<Char, Traits, Allocator>.
template <class Char, class Traits, class Allocator>
class SingleValueQueue<std::basic_string<Char, Traits, Allocator>> final
{
public:
	/// @brief the value type
	using value_type = std::basic_string<Char, Traits, Allocator>;

	/// @brief Enqueues a value.
	/// 
	/// Any value already in the queue will be replaced.
	/// @param value The value to place in the queue
	template <typename... Arguments> requires std::constructible_from<value_type, Arguments &&...>
	auto enqueue(Arguments &&... arguments) noexcept -> void
	{
		_value.store(std::make_unique<value_type>(std::forward<Arguments>(arguments)...), std::memory_order_release);
	}

	/// @brief Gets the last scheduled value and removes it from the queue
	/// @return The scheduled value or std::nullopt if none was scheduled since the last call
	auto dequeue() noexcept -> std::optional<value_type>;

private:
	/// @brief The queued value, or std::nullopt if the queue is empty.
	utils::atomic::UniquePointer<value_type> _value;
};

template <class Char, class Traits, class Allocator>
auto SingleValueQueue<std::basic_string<Char, Traits, Allocator>>::dequeue() noexcept -> std::optional<value_type>
{
	if (auto value = _value.exchange(nullptr, std::memory_order_acq_rel))
	{
		return std::move(*value);
	}
	else
	{
		return std::nullopt;
	}
}

} // namespace xentara::plugins::templateDriver