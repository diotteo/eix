// vim:set et cinoptions=g0,t0,^-2 sw=4 ts=4:
// This file is part of the eix project and distributed under the
// terms of the GNU General Public License v2.
//
// Copyright (c)
//   Emil Beinroth <emilbeinroth@gmx.net>

#ifndef __GUARD__FORMATED_H__
#define __GUARD__FORMATED_H__

#include <string>
#include <sstream>
#include <iostream>
#include <cassert>

namespace eix
{
  /** printf-like, typesafe text formating that replaces tokens in the given
   * string with textual representations of values.
   *
   * Specifier syntax is just like basic printf:
   * - @b %N is the specifier of type @b N
   * - @b %% is a literal @b %
   *
   * Recognized specifiers are:
   * - @b s  convert argument to string by using the <<-operator of std::ostream.
   * - @b r  like %s, but if argument is a string type (std::string or char *) it
   *         is enclosed in single quotes.
   */
  class format
  {
  public:
      /// Set the template string.
      explicit format(const std::string& format_string)
          : m_spec(0), m_stream(), m_format(format_string)
      { goto_next_spec(); }

      /// Copy current state into new formater.
      format(const format& e)
          : m_spec(e.m_spec), m_stream(e.m_stream.str()), m_format(e.m_format)
      { }

      /// Copy current state of formater.
      format& operator=(const format& e)
      {
          m_spec = e.m_spec;
          m_stream.str(e.m_stream.str());
          m_format = e.m_format;
          return *this;
      }

      /// Insert the value for the next placeholder.
      template<typename T>
          format& operator%(const T& s);

      /// Return the formated string.
      std::string str() const
      {
          assert(m_format.empty());
          return m_stream.str();
      }

      /// Convenience wrapper for str().
      operator std::string () 
      { return str(); }

      /// Write formated string to ostream os.
      friend std::ostream& operator<<(std::ostream& os, const format& formater)
      { return os << formater.str(); }

  protected:
      /// Find the next specifiers in the format string.
      void goto_next_spec();

      std::ostream& write_representation(std::ostream& s, const char *t)
      { return s << "'" << t << "'"; }

      std::ostream& write_representation(std::ostream& s, const std::string &t)
      { return s << "'" << t << "'"; }

      template<typename T>
          std::ostream& write_representation(std::ostream& s, const T& t)
          { return s << t; }

  private:
      /// Currently processed specifier, 0 if there are no more specifiers.
      char m_spec;
      /// The current state of the formated string.
      std::ostringstream m_stream;
      /// What is left of the format string.
      std::string m_format;
  };

  template<typename T>
      inline format &format::operator%(const T& s)
      {
          assert(m_spec != 0);
          assert(m_format.empty() != true);

          if (m_spec == 'r')
              write_representation(m_stream, s);
          else if (m_spec == 's')
              m_stream << s;
          else
              assert(! "unknown specifier");

          goto_next_spec();
          return *this;
      }

  inline void format::goto_next_spec()
  {
      m_spec = 0;
      std::string::size_type next = m_format.find('%');
      if (next == std::string::npos || m_format.size() < next+2) {
          // there are no more specifier, so we move the remaining text to our
          // stream.
          m_stream << m_format;
          m_format.clear();
      }
      else if (m_format.at(next+1) == '%') {
          // %% gives a single %
          m_stream << m_format.substr(0, next+1);
          m_format.erase(0, next+2);
          goto_next_spec();
      }
      else {
          // remember the specifier so we can use it in the next call to the
          // %-operator.
          m_spec = m_format.at(next+1);
          m_stream << m_format.substr(0, next);
          m_format.erase(0, next+2);
      }
  }
}
#endif /* __GUARD__FORMATED_H__ */
