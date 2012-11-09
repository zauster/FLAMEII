/*!
 * \file src/io/tests/test_io_xml_pop.cpp
 * \author Simon Coakley
 * \date 2012
 * \copyright Copyright (c) 2012 STFC Rutherford Appleton Laboratory
 * \copyright Copyright (c) 2012 University of Sheffield
 * \copyright GNU Lesser General Public License
 * \brief Test suite for the io xml pop method
 */
#define BOOST_TEST_DYN_LINK
#ifdef STAND_ALONE
#   define BOOST_TEST_MODULE IO Pop
#endif
#include <boost/test/unit_test.hpp>
#include <vector>
#include <string>
#include "../io_xml_model.hpp"
#include "../io_xml_pop.hpp"
#include "../../mem/memory_manager.hpp"

namespace xml = flame::io::xml;
namespace model = flame::model;

BOOST_AUTO_TEST_SUITE(IOPop)

/* Test creation of data schema */
BOOST_AUTO_TEST_CASE(test_data_schema) {
    int rc;
    xml::IOXMLPop ioxmlpop;
    xml::IOXMLModel ioxmlmodel;
    model::XModel model;

    /* Read model xml */
    ioxmlmodel.readXMLModel("src/io/tests/models/all_data.xml", &model);

    /* Generate data schema */
    rc = ioxmlpop.createDataSchema("src/io/tests/models/all_data.xsd", &model);
    BOOST_CHECK(rc == 0);

    /* Validate data using schema */
    std::string xsd = "src/io/tests/models/all_data.xsd";
    rc = ioxmlpop.validateData("src/io/tests/models/all_data_its/0.xml", xsd);
    BOOST_CHECK(rc == 0);
    /* Remove created 1.xml */
    if (remove(xsd.c_str()) != 0)
    fprintf(stderr, "Warning: Could not delete the generated file: %s",
            xsd.c_str());
}

/* Test the reading of XML population files. */
BOOST_AUTO_TEST_CASE(test_read_XML_model) {
    // int rc;
    xml::IOXMLPop ioxmlpop;
    xml::IOXMLModel ioxmlmodel;
    model::XModel model;
    // flame::mem::MemoryManager memoryManager;

    /* Read model xml */
//    ioxmlmodel.readXMLModel("src/io/tests/models/all_data.xml", &model);
    /* Register agents with mem module */
    unsigned int ii, jj;
    // size_t pop_size_hint = 3;
    for (ii = 0; ii < model.getAgents()->size(); ii++) {
        model::XMachine * agent = model.getAgents()->at(ii);
        /* Register agent */
        // memoryManager.RegisterAgent(agent->getName(), pop_size_hint);
        /* Register agent memory variables */
        for (jj = 0; jj < agent->getVariables()->size(); jj++) {
            model::XVariable * var =
                    agent->getVariables()->at(jj);
            if (var->getType() == "int") {
                /*memoryManager.RegisterAgentVar<int>
                    (agent->getName(), var->getName());*/
            } else if (var->getType() == "double") {
                /*memoryManager.RegisterAgentVar<double>
                    (agent->getName(), var->getName());*/
            }
        }
    }

    /* Warning: Using same memory manager for all tests */

    /*rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_missing.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 1);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_malformed.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 2);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_unknown_tag.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 3);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_unknown_agent.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 4);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_unknown_variable.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 5);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_var_not_int.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 6);

    rc = ioxmlpop.readXMLPop(
            "src/io/tests/models/all_data_its/0_var_not_double.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 6);

    rc = ioxmlpop.readXMLPop("src/io/tests/models/all_data_its/0.xml",
            &model, &memoryManager);
    BOOST_CHECK(rc == 0);*/

    /* Test pop data read in */
    /* Test ints data */
/*    flame::mem::VectorReader<int> roi =
            memoryManager.GetReader<int>("agent_a", "int_single");
    int expectedi[] = {1, 2, 3};
    BOOST_CHECK_EQUAL_COLLECTIONS(expectedi, expectedi+3,
            roi.begin(), roi.end());*/
    /* Test doubles data */
/*    flame::mem::VectorReader<double> rod =
            memoryManager.GetReader<double>("agent_a", "double_single");
    double expectedd[] = {0.1, 0.2, 0.3};
    for (ii = 0; ii < rod.size(); ii++) {
        BOOST_CHECK_CLOSE(*(rod.begin()+ii), *(expectedd+ii), 0.0001);
    }

    std::string onexml = "src/io/tests/models/all_data_its/1.xml";
    rc = ioxmlpop.writeXMLPop(onexml, 1, &model, &memoryManager);
    BOOST_CHECK(rc == 0);*/
    /* Remove created 1.xml */
/*    if (remove(onexml.c_str()) != 0)
        fprintf(stderr, "Warning: Could not delete the generated file: %s",
            onexml.c_str());*/
}

BOOST_AUTO_TEST_SUITE_END()