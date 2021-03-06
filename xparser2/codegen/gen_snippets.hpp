/*!
 * \file xparser2/code_generator.hpp
 * \author Shawn Chin
 * \date January 2013
 * \copyright Copyright (c) 2013 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2013 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Generators for short code snippets
 */
#ifndef XPARSER__CODEGEN__SNIPPETS_HPP_
#define XPARSER__CODEGEN__SNIPPETS_HPP_
#include <string>
#include <vector>
#include "code_generator.hpp"
namespace xparser { namespace codegen {

// Snippet classes defined further down this file
class RegisterAgentFuncSnippets;
class AgentFunctionHeaderSnippets;

/*! \brief Abstract base class for generator that handles single-line
 *         single-substitution code snippets
 *
 * Output is generated once for each var added.
 *
 * Unlike standard CodeGenerators, sub classes should not overload Generate().
 * Instead, they should overload GetSnippetText() and return the text to be
 * used for generating content (\c $VAR$ should be used as placeholder for
 * variable substitition).
 *
 * Sub classes should overload constructor and call RequireHeader() and/or
 * RequireSysHeader() to defined dependencies of the code snippet.
 *
 */
class SingleVarSnippet : public CodeGenerator {
  public:
    virtual ~SingleVarSnippet() {}

    //! Generates output text, once per added variablr
    inline void Generate(Printer* printer) const {
      const char* snippet = GetSnippetText();
      std::vector<std::string>::const_iterator i;
      for (i = vars_.begin(); i != vars_.end(); ++i) {
        printer->Print(snippet, "VAR", *i);
      }
    }

    //! Adds a single variable
    inline void Add(const std::string& var) {
      vars_.push_back(var);
    }

    //! Adds a vector of variables
    inline void Add(const std::vector<std::string>& var_vector) {
      vars_.insert(vars_.end(), var_vector.begin(), var_vector.end());
    }

  protected:
    //! Subclasses must overload this to provide text snippet
    virtual const char* GetSnippetText(void) const = 0;

  private:
    std::vector<std::string> vars_;  //! Vars to be used for producing output
};

//! Returns code to map an agent function pointer to the function name
class RegisterAgentFuncSnippets : public SingleVarSnippet {
  protected:
    const char* GetSnippetText(void) const {
      return "model.registerAgentFunction(\"$VAR$\", &$VAR$);\n";
    }
};

//! Returns code to declare an agent function in header
class AgentFunctionHeaderSnippets : public SingleVarSnippet {
  public:
    AgentFunctionHeaderSnippets() {
      RequireHeader("flame2.hpp");
    }

  protected:
    const char* GetSnippetText(void) const {
      return "FLAME_AGENT_FUNCTION($VAR$);\n";
    }
};



}}  // namespace xparser::codegen
#endif  // XPARSER__CODEGEN__SNIPPETS_HPP_
