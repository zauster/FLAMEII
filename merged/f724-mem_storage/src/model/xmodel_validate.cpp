/*!
 * \file src/model/xmodel_check.cpp
 * \author Simon Coakley
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Checks and validates a loaded model
 */
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#include "./model_manager.hpp"

namespace flame { namespace model {

int processVariable(XVariable * variable,
        XModel * model);
int processVariables(std::vector<XVariable*> * variables_,
        XModel * model);
int processAgentFunction(XFunction * function,
        std::vector<XVariable*> * variables);
int validateVariables(std::vector<XVariable*> * variables_,
        XModel * model, bool allowDyamicArrays);
int validateFunctionFile(std::string name);
int validateTimeunits(XTimeUnit * timeUnit, XModel * model);
int validateADT(XADT * adt, XModel * model);
int validateAgent(XMachine * agent, XModel * model);
int validateAgentFunction(XFunction * xfunction,
        XMachine * agent, XModel * model);
int validateAgentCommunication(XIOput * xioput, XMachine * agent,
        XModel * model);
int validateAgentConditionOrFilter(XCondition * xcondition, XMachine * agent,
        XMessage * xmessage, XModel * model);
int validateSort(XIOput * xioput, XMessage * xmessage);
int validateMessage(XMessage * xmessage, XModel * model);
bool name_is_allowed(std::string name);

/*!
 * \brief Validate the model
 * \return The number of errors found
 * \todo Check condition op IN has valid variables (if implemented).
 * \todo Check for no agents (if needed).
 * \todo Remove agents with no memory variables (if needed).
 * Detailed description.
 */
int XModel::validate() {
    int rc, errors = 0;
    unsigned int ii;

    /* Validate function files */
    for (ii = 0; ii < functionFiles_.size(); ii++) {
        rc = validateFunctionFile(functionFiles_.at(ii));
        errors += rc;
    }

    /* Validate data types */
    for (ii = 0; ii < adts_.size(); ii++) {
        rc = validateADT(adts_.at(ii), this);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from data type: '%s'.\n",
                adts_.at(ii)->getName().c_str());
            errors += rc;
        }
    }

    /* Validate model constants */
    rc = validateVariables(&constants_, this, false);
    if (rc != 0) {
        std::fprintf(stderr,
            "       from environment constants.\n");
        errors += rc;
    }

    /* Validate time units */
    for (ii = 0; ii < timeUnits_.size(); ii++) {
        rc = validateTimeunits(timeUnits_.at(ii), this);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from time unit: '%s'.\n",
                timeUnits_.at(ii)->getName().c_str());
            errors += rc;
        }
    }

    /* Validate agents */
    for (ii = 0; ii < agents_.size(); ii++) {
        rc = validateAgent(agents_.at(ii), this);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from agent: '%s'.\n",
                agents_.at(ii)->getName().c_str());
            errors += rc;
        }
    }

    /* Validate messages */
    for (ii = 0; ii < messages_.size(); ii++) {
        rc = validateMessage(messages_.at(ii), this);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from message: '%s'.\n",
                messages_.at(ii)->getName().c_str());
            errors += rc;
        }
    }

    /* If errors print out information */
    if (errors > 0) {
        std::fprintf(stderr, "%d error", errors);
        if (errors > 1) std::fprintf(stderr, "s");
        std::fprintf(stderr, " found.\n");
    }

    /* Return number of errors */
    return errors;
}

int validateAgent(XMachine * agent, XModel * model) {
    int rc, errors = 0;
    unsigned int ii;

    /* Check name is valid */
    if (!name_is_allowed(agent->getName())) {
        std::fprintf(stderr,
            "Error: Agent name is not valid: '%s',\n",
            agent->getName().c_str());
        errors++;
    }

    /* Check for duplicate names */
    for (ii = 0; ii < model->getAgents()->size(); ii++) {
        if (agent != model->getAgents()->at(ii) &&
                agent->getName() == model->getAgents()->at(ii)->getName()) {
            std::fprintf(stderr,
                "Error: Duplicate agent name: '%s',\n",
                agent->getName().c_str());
            errors++;
        }
    }

    rc = validateVariables(agent->getVariables(), model, true);
    errors += rc;

    for (ii = 0; ii < agent->getFunctions()->size(); ii++) {
        rc = validateAgentFunction(agent->getFunctions()->at(ii),
                agent, model);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from function: '%s',\n",
                agent->getFunctions()->at(ii)->getName().c_str());
            errors += rc;
        }
    }

    return errors;
}

int processVariable(XVariable * variable,
        XModel * model) {
    int errors = 0;
    unsigned int ii;

    /* Handle dynamic arrays in variable type */
    /* If the type can end in _array */
    if (variable->getType().size() > 6) {
        /* If the type ends in _array */
        if (variable->getType().compare(
                variable->getType().size()-6, 6, "_array") == 0) {
            /* Set dynamic array to be true */
            variable->setIsDynamicArray(true);
            /* Set that variable holds dynamic array */
            variable->setHoldsDynamicArray(true);
            /* Remove _array from end of type */
            variable->setType(
                variable->getType().erase(
                    variable->getType().size()-6, 6));
        }
    }

    /* Handle static arrays in variable name */
    int arraySize;
    /* Find brackets */
    size_t positionOfOpenBracket = variable->getName().find("[");
    size_t positionOfCloseBracket = variable->getName().find("]");
    /* If brackets found */
    if (positionOfOpenBracket != std::string::npos &&
        positionOfCloseBracket != std::string::npos) {
        /* If close bracket is at the end of the name */
        if (positionOfCloseBracket == variable->getName().size()-1) {
            /* Set static array to be true */
            variable->setIsStaticArray(true);
            /* Convert array size number to integer */
            std::string arraySizeString =
                    variable->getName().substr(positionOfOpenBracket+1,
                    positionOfCloseBracket-positionOfOpenBracket-1);
            bool castSuccessful = true;
            try {
                arraySize = boost::lexical_cast<int>(arraySizeString);
            } catch(const boost::bad_lexical_cast&) {
                std::fprintf(stderr,
                    "Error: Static array number not an integer: '%s'\n",
                    arraySizeString.c_str());
                errors++;
                castSuccessful = false;
            }
            /* If cast was successful */
            if (castSuccessful) {
                if (arraySize < 1) {
                    std::fprintf(stderr,
                        "Error: Static array size is not valid: '%d'\n",
                        arraySize);
                    errors++;
                }
                /* Set successful array size integer */
                variable->setStaticArraySize(arraySize);
            }

            /* Remove bracket from name */
            variable->setName(variable->getName().erase(positionOfOpenBracket));
        }
    }

    /* Check if data type is a user defined data type */
    for (ii = 0; ii < model->getADTs()->size(); ii++) {
        if (variable->getType() == model->getADTs()->at(ii)->getName()) {
            variable->setHasADTType(true);
            /* If the ADT holds dynamic arrays then set this variable */
            if (model->getADTs()->at(ii)->holdsDynamicArray())
                variable->setHoldsDynamicArray(true);
        }
    }

    /* Validate constant if set */
    if (variable->isConstantSet()) {
        if (variable->getConstantString() == "true") {
            variable->setConstant(true);
        } else if (variable->getConstantString() == "false") {
            variable->setConstant(false);
        } else {
            std::fprintf(stderr, "Error: variable constant value ");
            std::fprintf(stderr, "is not 'true' or 'false': '%s',\n",
                    variable->getConstantString().c_str());
            errors++;
        }
    }

    return errors;
}

int processVariables(std::vector<XVariable*> * variables,
        XModel * model) {
    int rc, errors = 0;
    unsigned int ii;

    /* Handle dynamic arrays in variable type */
    for (ii = 0; ii < variables->size(); ii++) {
        rc = processVariable(variables->at(ii), model);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from variable: '%s %s',\n",
                variables->at(ii)->getType().c_str(),
                variables->at(ii)->getName().c_str());
            errors += rc;
        }
    }

    return errors;
}

int processAgentFunction(XFunction * function,
        std::vector<XVariable*> * variables) {
    std::vector<XVariable*>::iterator variable;

    /* If memory access information was not given set all memory
     * variable access as being read write */
    if (!function->getMemoryAccessInfoAvailable()) {
        for (variable = variables->begin();
                variable != variables->end(); ++variable) {
            function->addReadWriteVariable((*variable));
        }
    }

    return 0;
}

int validateVariables(std::vector<XVariable*> * variables,
        XModel * model, bool allowDyamicArrays) {
    int rc, errors = 0;
    unsigned int ii, jj;
    bool foundValidDataType;

    /* Process variables first */
    rc = processVariables(variables, model);
    errors += rc;

    /* Check names are valid */
    for (ii = 0; ii < variables->size(); ii++) {
        if (!name_is_allowed(variables->at(ii)->getName())) {
            std::fprintf(stderr,
                "Error: Variable name is not valid: '%s',\n",
                variables->at(ii)->getName().c_str());
            errors++;
        }
    }

    /* Check for duplicate names */
    for (ii = 0; ii < variables->size(); ii++) {
        for (jj = 0; jj < variables->size(); jj++) {
            if (variables->at(ii) != variables->at(jj) &&
                variables->at(ii)->getName() ==
                variables->at(jj)->getName()) {
                std::fprintf(stderr,
                    "Error: Duplicate variable name: '%s',\n",
                    variables->at(ii)->getName().c_str());
                errors++;
            }
        }
    }

    /* Check for valid types */
    for (ii = 0; ii < variables->size(); ii++) {
        foundValidDataType = false;
        /* Check single data types */
        for (jj = 0; jj < model->getAllowedDataTypes()->size(); jj++) {
            if (variables->at(ii)->getType() ==
                    model->getAllowedDataTypes()->at(jj))
                foundValidDataType = true;
        }
        if (!foundValidDataType) {
            std::fprintf(stderr,
                "Error: Data type: '%s' not valid for variable name: '%s',\n",
                variables->at(ii)->getType().c_str(),
                variables->at(ii)->getName().c_str());
            errors++;
        } else {
            /* Check if data type is a user defined data type */
        }
        /* Check for allowed dynamic arrays */
        if (!allowDyamicArrays) {
            if (variables->at(ii)->holdsDynamicArray()) {
                if (variables->at(ii)->isDynamicArray()) {
                    std::fprintf(stderr,
                        "Error: Dynamic array not allowed: '%s_array %s',\n",
                        variables->at(ii)->getType().c_str(),
                        variables->at(ii)->getName().c_str());
                } else {
                    std::fprintf(stderr,
                "Error: Dynamic array (in data type) not allowed: '%s %s',\n",
                        variables->at(ii)->getType().c_str(),
                        variables->at(ii)->getName().c_str());
                }
                errors++;
            }
        }
    }

    return errors;
}

int validateFunctionFile(std::string name) {
    int errors = 0;

    /* Name ends in '.c' */
    if (!boost::algorithm::ends_with(name, ".c")) {
        std::fprintf(stderr,
            "Error: Function file does not end in '.c': '%s',\n",
            name.c_str());
        errors++;
    }

    return errors;
}

int validateTimeunits(XTimeUnit * timeUnit, XModel * model) {
    int errors = 0;
    unsigned int ii;

    /* Check name is valid */
    if (!name_is_allowed(timeUnit->getName())) {
        std::fprintf(stderr,
            "Error: Time unit name is not valid: '%s',\n",
            timeUnit->getName().c_str());
        errors++;
    }

    /* Check for duplicate names */
    for (ii = 0; ii < model->getTimeUnits()->size(); ii++) {
        if (timeUnit != model->getTimeUnits()->at(ii) &&
                timeUnit->getName() ==
                        model->getTimeUnits()->at(ii)->getName()) {
            std::fprintf(stderr,
                "Error: Duplicate time unit name: '%s',\n",
                timeUnit->getName().c_str());
            errors++;
        }
    }

    /* Check that name is not iteration */
    if (timeUnit->getName() == "iteration") {
        std::fprintf(stderr,
            "Error: Time unit name cannot be 'iteration',\n");
        errors++;
    }

    /* Check unit is valid */
    bool unitIsValid = false;
    if (timeUnit->getUnit() == "iteration") unitIsValid = true;
    for (ii = 0; ii < model->getTimeUnits()->size(); ii++) {
        if (timeUnit != model->getTimeUnits()->at(ii) &&
                timeUnit->getUnit() == model->getTimeUnits()->at(ii)->getName())
            unitIsValid = true;
    }
    if (!unitIsValid) {
        std::fprintf(stderr,
                "Error: Time unit unit is not valid: '%s',\n",
                timeUnit->getUnit().c_str());
            errors++;
    }

    /* Process period and validate */
    bool castSuccessful = true;
    int period;
    try {
        period = boost::lexical_cast<int>(
                timeUnit->getPeriodString());
    } catch(const boost::bad_lexical_cast&) {
        std::fprintf(stderr,
            "Error: Period number not an integer: '%s'\n",
            timeUnit->getPeriodString().c_str());
        errors++;
        castSuccessful = false;
    }
    /* If cast was successful */
    if (castSuccessful) {
        if (period < 1) {
            std::fprintf(stderr,
                "Error: Period value is not valid: '%d'\n",
                period);
            errors++;
        }
        /* Set successful array size integer */
        timeUnit->setPeriod(period);
    }

    return errors;
}

int validateADT(XADT * adt, XModel * model) {
    int rc, errors = 0;
    unsigned int jj;
    bool dataTypeNameIsValid;

    /* Check name is valid */
    dataTypeNameIsValid = name_is_allowed(adt->getName());
    if (!dataTypeNameIsValid) {
        std::fprintf(stderr,
            "Error: Data type name is not valid: '%s',\n",
            adt->getName().c_str());
        errors++;
    }

    /* Check ADT name is not already a valid data type */
    for (jj = 0; jj < model->getAllowedDataTypes()->size(); jj++) {
        if (adt->getName() ==
                model->getAllowedDataTypes()->at(jj)) {
            dataTypeNameIsValid = false;
            std::fprintf(stderr,
                "Error: Data type already exists: '%s',\n",
                adt->getName().c_str());
            errors++;
        }
    }

    if (dataTypeNameIsValid) {
        /* Add data type to list of model allowed data types */
        model->addAllowedDataType(adt->getName());

        rc = validateVariables(adt->getVariables(), model, true);
        errors += rc;

        /* Check if adt contains dynamic arrays */
        for (jj = 0; jj < adt->getVariables()->size(); jj++) {
            if (adt->getVariables()->at(jj)->holdsDynamicArray())
                adt->setHoldsDynamicArray(true);
        }
    }

    return errors;
}

int validateAgentFunction(XFunction * xfunction, XMachine * agent,
        XModel * model) {
    int rc, errors = 0;
    unsigned int kk;

    /* Process function first */
    processAgentFunction(xfunction, agent->getVariables());

    /* Check name is valid */
    if (!name_is_allowed(xfunction->getName())) {
        std::fprintf(stderr,
            "Error: Function name is not valid: '%s',\n",
            xfunction->getName().c_str());
        errors++;
    }

    /* Check current state name is valid */
    if (!name_is_allowed(xfunction->getCurrentState())) {
        std::fprintf(stderr,
            "Error: Function current state name is not valid: '%s',\n",
            xfunction->getCurrentState().c_str());
        errors++;
    }

    /* Check next state name is valid */
    if (!name_is_allowed(xfunction->getNextState())) {
        std::fprintf(stderr,
            "Error: Function next state name is not valid: '%s',\n",
            xfunction->getNextState().c_str());
        errors++;
    }

    /* If condition then process and validate */
    if (xfunction->getCondition() != 0) {
        rc = validateAgentConditionOrFilter(xfunction->getCondition(),
                agent, 0, model);
        errors += rc;
    }

    /* For each input */
    for (kk = 0; kk < xfunction->getInputs()->size(); kk++) {
        rc = validateAgentCommunication(xfunction->getInputs()->at(kk),
                agent, model);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from input of message: '%s',\n",
                xfunction->getInputs()->at(kk)->getMessageName().c_str());
            errors += rc;
        }
    }

    /* For each output */
    for (kk = 0; kk < xfunction->getOutputs()->size(); kk++) {
        rc = validateAgentCommunication(xfunction->getOutputs()->at(kk),
                agent, model);
        if (rc != 0) {
            std::fprintf(stderr,
                "       from output of message: '%s',\n",
                xfunction->getInputs()->at(kk)->getMessageName().c_str());
            errors += rc;
        }
    }

    return errors;
}

int validateAgentCommunication(XIOput * xioput, XMachine * agent,
        XModel * model) {
    int rc, errors = 0;
    XMessage * xmessage;

    xmessage = model->getMessage(xioput->getMessageName());

    /* Check message type */
    if (xmessage == 0) {
        std::fprintf(stderr, "Error: message name is not valid: '%s',\n",
                xioput->getMessageName().c_str());
        errors++;
    }

    /* If filter then process and validate */
    if (xioput->getFilter() != 0) {
        rc = validateAgentConditionOrFilter(xioput->getFilter(), agent,
                model->getMessage(xioput->getMessageName()), model);
        errors += rc;
    }

    /* If sort then validate */
    if (xioput->isSort()) {
        rc = validateSort(xioput, xmessage);
        errors += rc;
    }

    /* If random then validate */
    if (xioput->isRandomSet()) {
        if (xioput->getRandomString() == "true") {
            xioput->setRandom(true);
        } else if (xioput->getRandomString() == "false") {
            xioput->setRandom(false);
        } else {
std::fprintf(stderr, "Error: input random is not 'true' or 'false': '%s',\n",
                    xioput->getRandomString().c_str());
            errors++;
        }
    }

    /* Cannot be sorted and random at the same time */
    if (xioput->isSort() && xioput->isRandom()) {
std::fprintf(stderr, "Error: input cannot be sorted and random too: '%s',\n",
                xioput->getMessageName().c_str());
        errors++;
    }

    return errors;
}

int validateAgentConditionOrFilter(XCondition * xcondition, XMachine * agent,
        XMessage * xmessage, XModel * model) {
    int rc, errors = 0;

    rc = xcondition->processSymbols();
    errors += rc;
    rc = xcondition->validate(agent, xmessage, model);
    errors += rc;

    return errors;
}

int validateSort(XIOput * xioput, XMessage * xmessage) {
    int errors = 0;

    /* Validate key as a message variable */
    if (xmessage != 0) {
        if (!xmessage->validateVariableName(xioput->getSortKey())) {
            std::fprintf(stderr,
                "Error: sort key is not a valid message variable: '%s',\n",
                xioput->getSortKey().c_str());
            errors++;
        }
    } else {
       std::fprintf(stderr,
    "Error: cannot validate sort key as the message type is invalid: '%s',\n",
           xioput->getSortKey().c_str());
       errors++;
    }
    /* Validate order as ascending or descending */
    if (xioput->getSortOrder() != "ascend" &&
            xioput->getSortOrder() != "descend") {
        std::fprintf(stderr,
            "Error: sort order is not 'ascend' or 'descend': '%s',\n",
            xioput->getSortOrder().c_str());
        errors++;
    }

    return errors;
}

int validateMessage(XMessage * xmessage, XModel * model) {
    int rc, errors = 0;
    unsigned int ii;

    /* Check name is valid */
    if (!name_is_allowed(xmessage->getName())) {
        std::fprintf(stderr,
            "Error: Message name is not valid: '%s',\n",
            xmessage->getName().c_str());
        errors++;
    }

    /* Check for duplicate names */
    for (ii = 0; ii < model->getMessages()->size(); ii++) {
        if (xmessage != model->getMessages()->at(ii) &&
                xmessage->getName() ==
                        model->getMessages()->at(ii)->getName()) {
            std::fprintf(stderr,
                "Error: Duplicate message name: '%s',\n",
                xmessage->getName().c_str());
            errors++;
        }
    }

    rc = validateVariables(xmessage->getVariables(), model, false);
    errors += rc;

    return errors;
}

bool char_is_disallowed(char c) {
    return !(isalnum(c) || c == '_' || c == '-');
}

/*!
 * \brief Validates a name string
 * \param[in] name The string to validate
 * \return Boolean result
 * The iterator tries to find the first element that satisfies the predicate.
 * If no disallowed character is found then you end with name.end() which is
 * taken to be success and true is returned.
 */
bool name_is_allowed(std::string name) {
    return std::find_if(name.begin(), name.end(),
            char_is_disallowed) == name.end();
}

}}  // namespace flame::model