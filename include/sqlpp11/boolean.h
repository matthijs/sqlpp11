/*
 * Copyright (c) 2013-2014, Roland Bock
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SQLPP_BOOLEAN_H
#define SQLPP_BOOLEAN_H

#include <cstdlib>
#include <ostream>
#include <sqlpp11/basic_expression_operators.h>
#include <sqlpp11/type_traits.h>
#include <sqlpp11/exception.h>

namespace sqlpp
{
	// boolean operators
	namespace detail
	{
		// boolean value type
		struct boolean
		{
			using _traits = make_traits<boolean, ::sqlpp::tag::is_boolean, ::sqlpp::tag::is_expression>;
			using _tag = ::sqlpp::tag::is_boolean;
			using _cpp_value_type = bool;

			struct _parameter_t
			{
				using _value_type = boolean; // FIXME

				_parameter_t():
					_value(false),
					_is_null(true)
				{}

				_parameter_t(const _cpp_value_type& value):
					_value(value),
					_is_null(false)
				{}

				_parameter_t& operator=(const _cpp_value_type& value)
				{
					_value = value;
					_is_null = (false);
					return *this;
				}

				_parameter_t& operator=(const std::nullptr_t&)
				{
					_value = false;
					_is_null = true;
					return *this;
				}

				bool is_null() const
				{ 
					return _is_null; 
				}

				_cpp_value_type value() const
				{
					return _value;
				}

				operator _cpp_value_type() const { return value(); }

				template<typename Target>
					void _bind(Target& target, size_t index) const
					{
						target._bind_boolean_parameter(index, &_value, _is_null);
					}

			private:
				signed char _value;
				bool _is_null;
			};

			template<typename Db, typename FieldSpec>
				struct _result_field_t;

			// I am SO waiting for concepts lite!
			template<typename Field, typename Enable = void>
				struct field_methods_t
				{
					static constexpr bool _null_is_trivial = true;
					operator _cpp_value_type() const { return static_cast<const Field&>(*this).value(); }
				};

			template<typename Db, typename FieldSpec>
				struct field_methods_t<
						_result_field_t<Db, FieldSpec>, 
				    typename std::enable_if<enforce_null_result_treatment_t<Db>::value 
							and column_spec_can_be_null_t<FieldSpec>::value
							and not null_is_trivial_value_t<FieldSpec>::value>::type>
				{
					static constexpr bool _null_is_trivial = false;
				};

			template<typename Db, typename FieldSpec>
				struct _result_field_t: public field_methods_t<_result_field_t<Db, FieldSpec>>
				{
					using _field_methods_t = field_methods_t<_result_field_t<Db, FieldSpec>>;

					using _traits = make_traits<integral,
								tag_if<tag::null_is_trivial_value, _field_methods_t::_null_is_trivial>>;

					struct _recursive_traits
					{
						using _parameters = std::tuple<>;
						using _provided_tables = detail::type_set<>;
						using _provided_outer_tables = detail::type_set<>;
						using _required_tables = detail::type_set<>;
						using _extra_tables = detail::type_set<>;
						using _can_be_null = column_spec_can_be_null_t<FieldSpec>;
					};

					_result_field_t():
						_is_valid(false),
						_is_null(true),
						_value(false)
					{}

					void _validate()
					{
						_is_valid = true;
					}

					void _invalidate()
					{
						_is_valid = false;
						_is_null = true;
						_value = 0;
					}

					bool is_null() const
					{ 
						if (not _is_valid)
							throw exception("accessing is_null in non-existing row");
						return _is_null; 
					}

					_cpp_value_type value() const
					{
						if (not _is_valid)
							throw exception("accessing value in non-existing row");

						if (_is_null)
						{
							if (enforce_null_result_treatment_t<Db>::value and not null_is_trivial_value_t<FieldSpec>::value)
							{
								throw exception("accessing value of NULL field");
							}
							else
							{
								return false;
							}
						}
						return _value;
					}

					template<typename Target>
						void _bind(Target& target, size_t i)
						{
							target._bind_boolean_result(i, &_value, &_is_null);
						}

				private:
					bool _is_valid;
					bool _is_null;
					signed char _value;
				};

			template<typename T>
				struct _is_valid_operand
				{
					static constexpr bool value = 
						is_expression_t<T>::value // expressions are OK
						and is_boolean_t<T>::value // the correct value type is required, of course
						;
				};

			template<typename Base>
				struct expression_operators: public basic_expression_operators<Base, is_boolean_t>
			{
				template<typename T>
					logical_and_t<Base, wrap_operand_t<T>> operator and(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), rhs{t} };
					}

				template<typename T>
					logical_or_t<Base, wrap_operand_t<T>> operator or(T t) const
					{
						using rhs = wrap_operand_t<T>;
						static_assert(_is_valid_operand<rhs>::value, "invalid rhs operand");

						return { *static_cast<const Base*>(this), rhs{t} };
					}

				logical_not_t<Base> operator not() const
				{
					return { *static_cast<const Base*>(this) };
				}
			};

			template<typename Base>
				struct column_operators
				{
				};
		};

		template<typename Db, typename FieldSpec>
			inline std::ostream& operator<<(std::ostream& os, const boolean::_result_field_t<Db, FieldSpec>& e)
			{
				return os << e.value();
			}
	}

	using boolean = detail::boolean;

}
#endif
