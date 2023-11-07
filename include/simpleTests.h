﻿#pragma once

#include <functional>
#include <string>
#include <vector>
#include <queue>
#include <iostream>
#include <sstream>

// # SimpleTests
// Simple single header C++ test framework
// 
// Simple Tests is a simple single header testing library that I made because I'm too stupid to use Catch2, googletest, 
// or the other cool testing frameworks.
// 
// # Installation instructions
// Download `SimpleTests/simpleTests.h` and include it in your project and build system
// 
// # How - to:
// ## Declare a test
// ```c+ +
// #include "simpleTest.h"
// 
// ST_TEST(ATestName, "Some description on what the test does")
// {
//     ST_ASSERT(true, "this test cannot fail.")
// }
// ```
// 
// ## Declare a section
// ```c+ +
// #include "simpleTest.h"
// 
// ST_SECTION_BEGIN(MySection, "My section's description")
// {
//     ST_BEFORE_EACH(MyBeforeEachFunctor)
//     {
//         // Initialize my tests' context
//     }
// 
//     ST_TEST(MyTest, "My test's description")
//     {
//         ST_ASSERT(true, "all is good.");
//     }
// 
//     ST_AFTER_EACH(MyAfterEachFunctor)
//     {
//         // clean up my tests' context
//     }
// }
// ST_SETION_END(MySection)
// ```
// ## Run a single Test or Section
// Use `ST_TEST_ONLY` and /or `ST_SECTION_BEGIN_ONLY` to isolate tests.This is useful when debugging a single test or section.There's no need to run all the tests everytime if you're iterating.
// ```c+ +
// #include "simpleTests.h"
// 
// ST_SECTION_BEGIN(OnlySection, "Test only flow")
// {
//     // this test will run
//     ST_TEST_ONLY(OnlyTest, "OnlyTest, should pass")
//     {
//         ST_ASSERT(true, "assert true");
//     }
// 
//     // this test will not run
//     ST_TEST(NotOnlyTest, "Should not run, if it does, fails.")
//     {
//         ST_ASSERT(false, "assert false");
//     }
// }
// ST_SECTION_END(OnlySection)
// 
// ST_SECTION_BEGIN(OnlySubSection, "Test only flow with subsections")
// {
//     // this whole section will run
//     ST_SECTION_BEGIN_ONLY(OnlySubSubSection, "Test only flow with subsections")
//     {
//         ST_TEST(TestInOnlySubsection, "Only subsection Test, should pass")
//         {
//             ST_ASSERT(true, "assert true");
//         }
//     }
//     ST_SECTION_END(OnlySubSubSection)
// }
// ST_SECTION_END(OnlySubSection)
// ```
// 
// ## Run all tests
// ```c+ +
// int main()
// {
//     return SimpleTests::SimpleTestsRunner::runTests();
// }
// ```
// 
// ## Exemple output :
// ```
// [--------] Global
// [ FAILED ] |--Test1: should fail
// [ ASSERT ] |--[ASSERT] Failed : assert false (C:\Repository\SimpleTests\SampleProject\main.cpp:5)
// [   ok   ] |--Test2 : should pass
// [   ok   ] |--SomeNotFancyTest : Should not be initialized
// [   ok   ] |--SomeTestInAnotherFile2 : should pass
// [--------] Section1 : This is the first section
// [   ok   ] |--SomeTestInASection : should pass
// [--------] Section2 : This is another section, more to come
// [--------] |--Section2_1 : this is a subsection
// [   ok   ] |-- |--SomeTestInASubSection : should fail
// [--------] |-- |--Section2_1_1 : yet another subsection
// [   ok   ] |-- |-- |--SomeTestInSection2_1_1 : Should pass
// [--------] SomeFancySection : This is the fancy section
// [   ok   ] |--SomeFancyTest : Should be initialized
// 
// 
// 7 ouf of 8 tests passed.
// ```



/*
 * ###############################################################
 * Simple Tests Core
 * ###############################################################
 */
namespace SimpleTests
{
// Colors
#define RED         "\033[31m"      /* Red */
#define GREEN       "\033[32m"      /* Green */
#define BOLD        "\033[1m"
#define RESET       "\033[0m"

// labels
#define ST_DASHES_STRING "[--------] "
#define ST_PASSED_STRING "[   ok   ] "
#define ST_FAILED_STRING "[ FAILED ] "
#define ST_ASSERT_STRING "[ ASSERT ] "

    // Test container
    struct SimpleTest
    {
        SimpleTest(const std::string& inTitle, const std::string& inDescription,  std::function<void()> inF, bool inIsOnly)
            : title(inTitle), description(inDescription), f(inF), isOnly(inIsOnly) {}

        std::string title;
        std::string description;
        std::function<void()> f;
        bool hasPassed = false; // has the test passed
        bool isOnly = false; // is marked for isOnly flow
        bool isAllowedToRun = false; // is allowed to run after the isOnly

        void addAssertLog(const std::string& assertLog)
        {
            assertLogs.push_back(assertLog);
        }

        std::string getLog(const std::string& indent) const
        {
            std::stringstream testStream;
            // if atleast one assert log was pushed in the s_assertLogs, the test has failed.
            if (!hasPassed)
            {
                testStream << BOLD << RED << ST_FAILED_STRING << RED << indent << title << ": " << description << RESET << "\n";
                for (const std::string& assertLog : assertLogs)
                {
                    testStream << RED << ST_ASSERT_STRING << indent << assertLog << RESET;
                }
            }
            else
            {
                testStream << BOLD << GREEN << ST_PASSED_STRING << RESET << indent << title << ": " << description << "\n";
            }

            return testStream.str();
        }
    private:
        std::vector<std::string> assertLogs;
    };

    // section container
    struct Section
    {
        Section(const std::string& inTitle, const std::string& inDescription, bool inIsOnly)
          : title(inTitle), description(inDescription), isOnly(inIsOnly)
        {
        };

        std::string title;
        std::string description;
        std::function<void()> onBeforeEach;
        std::function<void()> onAfterEach;

        std::vector<SimpleTest> tests;
        std::vector<Section> children;

        bool hasPassed = false;
        bool isOnly = false;
        bool isAllowedToRun = false;

        std::string getLog(const std::string& indent) const
        {
            // display the section title and description
            std::stringstream sectionTitle;
            sectionTitle << (hasPassed ? GREEN : RED) << ST_DASHES_STRING << RESET;
            sectionTitle << indent << BOLD << title << (description.empty() ? "" : ": " + description) << RESET << "\n";
            return sectionTitle.str();
        }
    };

    // Simple's test context. this struct exposes all the Simple tests' state as static function. This way they're initialized when we call
    // the getter for the first time, avoiding initialization order bugs. 
    struct SimpleTestsContext
    {
        // registers a new test case
        static void registerTest(const std::string& title, const std::string& description, std::function<void()> func, bool isOnly)
        {
            getTests().push_back({title, description, func, isOnly});
        }

        // pushes an assert message in the assert logs queue. it will be consumed once the current test is completed
        static void notifyAssertFailed(const std::string& message)
        {
            getAssertLogs().push(message);
        }

        static void beginSection(const std::string& title, const std::string& description, bool isOnly)
        {
            Section s { title, description, isOnly };
            Section* current = getSectionStack().back();
            current->children.push_back(std::move(s));
            
            // this is safe because the parent cannot reallocate its children vector unless it's the current scope.
            getSectionStack().push_back(&current->children.back());
        }

        static void registerBeforeEach(std::function<void()> f)
        {
            getSectionStack().back()->onBeforeEach = f;
        }

        static void registerAfterEach(std::function<void()> f)
        {
            getSectionStack().back()->onAfterEach = f;
        }

        static void endSection()
        {
            SimpleTestsContext::getSectionStack().pop_back();
        }

        static std::queue<std::string>& getAssertLogs() 
        {
            static std::queue<std::string> s_assertLogs;
            return s_assertLogs;
        }

        static std::vector<Section*>& getSectionStack()
        {
            static std::vector<Section*> s_sectionStack = { &getGlobalSection() };
            return s_sectionStack;
        }

        static Section& getGlobalSection()
        {
            static Section s_global{ "Global", "", false};
            return s_global;
        }

        static uint32_t& getFailedTestCounter()
        {
            static uint32_t s_failedTestCounter;
            return s_failedTestCounter;
        }

    private:
        static std::vector<SimpleTest>& getTests()
        {
            return getSectionStack().back()->tests;
        }
    };

    struct SimpleTestsRunner
    {
        // Executes all tests in s_tests
        static int runTests()
        {
            Section global = SimpleTestsContext::getGlobalSection();
            
            const bool shouldCheckIsOnly = hasIsOnly(global);
            processIsAllowedToRunFlag(global, shouldCheckIsOnly, global.isOnly);

            std::vector<const Section*> sectionStack;
            runSection(global, sectionStack, true);
            logSection(global, sectionStack, true);

            size_t totalTests = 0;
            size_t failedTests = 0;
            countTests(global, totalTests, failedTests);
            // display the final test success count.
            std::cout << "\n\n" << totalTests - failedTests << " ouf of " << totalTests << " test";
            if (totalTests > 1)
            {
                // I'm a gentleman.
                std::cout << "s";
            }
            std::cout <<" passed." << "\n";

            return failedTests > 0 ? 0 : -1;
        }

        static bool hasIsOnly(const Section& section)
        {
            if (section.isOnly)
            {
                return true;
            }
            
            for (const Section& child : section.children)
            {
                if (hasIsOnly(child))
                {
                    return true;
                }
            }
            
            for (const SimpleTest& test : section.tests)
            {
                if (test.isOnly)
                {
                    return true;
                }
            }

            return false;
        }

        static bool processIsAllowedToRunFlag(Section& section, const bool shouldCheckIsOnly, const bool isParentOnly)
        {
            if (!shouldCheckIsOnly)
            {
                // there are no isOnly tests or sections, allow everything to run.
                section.isAllowedToRun = true;
                for (SimpleTest& test : section.tests)
                {
                    test.isAllowedToRun = true;
                }

                for (Section& child : section.children)
                {
                    processIsAllowedToRunFlag(child, shouldCheckIsOnly, true || isParentOnly);
                }

                return true;
            }
            else
            {
                // there's at least one isOnly section or test in the section tree. Check it the section is allowed to run.
                // if the section is only or one of its parent is only, it is allowed to run.
                bool isAllowedToRun = section.isOnly || isParentOnly;
                const bool isSectionAllowedToRunBeforeCheckingSubSection = isAllowedToRun;
                for (SimpleTest& test : section.tests)
                {
                    // if the section is allowed to run or the test is only, allow the test to run
                    test.isAllowedToRun = isSectionAllowedToRunBeforeCheckingSubSection || test.isOnly;
                    // if the test is allowed to run, the section is allowed to run
                    isAllowedToRun |= test.isAllowedToRun;
                }
                
                for (Section& child : section.children)
                {
                    // if a child section is allowed to run, this section is allowed to run
                    isAllowedToRun |= processIsAllowedToRunFlag(child, shouldCheckIsOnly, section.isOnly || isParentOnly);
                }

                section.isAllowedToRun = isAllowedToRun;
                // trickle the isAllowedToRun down.
                return isAllowedToRun;
            }
        }
    
        static bool runSection(Section& section, std::vector<const Section*>& sectionStack, const bool isInGlobalScope)
        {
            if (!section.isAllowedToRun)
            {
                return true;
            }

            bool hasPassed = true;
            sectionStack.push_back(&section);

            // the this section's tests
            hasPassed &= runTests(section.tests, sectionStack);

            for (Section& child : section.children)
            {
                // run the children sections
                hasPassed &= runSection(child, sectionStack, false);
            }

            //std::cout << ST_DASHES_STRING << "\n";
            sectionStack.pop_back();
            section.hasPassed = hasPassed;
            return hasPassed;
        }

        static bool runTests(std::vector<SimpleTest>& tests, const std::vector<const Section*>& sectionStack)
        {
            bool hasPassed = true;
            for (auto& test : tests)
            {
                if (!test.isAllowedToRun)
                {
                    continue;
                }

                // run the test
                for (const Section* section : sectionStack)
                {
                    if (section->onBeforeEach)
                    {
                        section->onBeforeEach();
                    }
                }

                test.f();

                for (const Section* section : sectionStack)
                {
                    if (section->onAfterEach)
                    {
                        section->onAfterEach();
                    }
                }

                auto& assertLogs = SimpleTestsContext::getAssertLogs();

                test.hasPassed = assertLogs.size() == 0;
                hasPassed &= test.hasPassed;
                if (assertLogs.size() > 0)
                {
                    // increment the failed test counter for the final display
                    SimpleTestsContext::getFailedTestCounter()++;

                    // dump all assert logs in the test logs queue (after the title)
                    while (assertLogs.size() > 0)
                    {
                        std::stringstream ss;
                        ss << RED << ST_ASSERT_STRING << assertLogs.front() << RESET;
                
                        test.addAssertLog(ss.str());
                        assertLogs.pop();
                    }
                }
            }
            return hasPassed;
        }

        static void logSection(const Section& section, std::vector<const Section*>& sectionStack, bool isInGlobalScope)
        {
            if (!section.isAllowedToRun)
            {
                return;
            }

            sectionStack.push_back(&section);

            std::string indent = "";
            if (sectionStack.size() > 1)
            {
                for (size_t i = 2; i < sectionStack.size(); ++i)
                {
                    indent += "|--";
                }
            }

            std::cout << section.getLog(indent);

            if (sectionStack.size() > 0)
            {
                indent += "|--";
            }

            for (const auto& test : section.tests)
            {
                if (!test.isAllowedToRun)
                {
                    continue;
                }

                std::cout << test.getLog(indent);
            }

            for (const auto& child : section.children)
            {
                logSection(child, sectionStack, false);
            }

            sectionStack.pop_back();
        }

        static void countTests(const Section& section, size_t& outTotalTestCount, size_t& outFailedTotalTestCount)
        {
            if (!section.isAllowedToRun)
            {
                return;
            }

            for (const Section& child : section.children)
            {
                countTests(child, outTotalTestCount, outFailedTotalTestCount);
            }

            for (const SimpleTest& test : section.tests)
            {
                if (!test.isAllowedToRun)
                {
                    continue;
                }
                
                outTotalTestCount++;

                if (!test.hasPassed)
                {
                    outFailedTotalTestCount++;
                }
            }
        }
    };

    struct AutoRegister
    {
        AutoRegister(const std::string& title, const std::string& description, std::function<void()> func, bool isOnly = false)
        {
            SimpleTestsContext::registerTest(title, description, func, isOnly);
        };
    };

    struct SectionBeginner
    {
        SectionBeginner(const std::string& title, const std::string& description, bool isOnly = false)
        {
            SimpleTestsContext::beginSection(title, description, isOnly);
        }
    };

    struct SectionBeforeEachRegister
    {
        SectionBeforeEachRegister(std::function<void()> f)
        {
            SimpleTestsContext::registerBeforeEach(f);
        }
    };

    struct SectionAfterEachRegister
    {
        SectionAfterEachRegister(std::function<void()> f)
        {
            SimpleTestsContext::registerAfterEach(f);
        }
    };

    struct SectionEnder
    {
        SectionEnder()
        {
            SimpleTestsContext::endSection();
        }
    };
}

/*
 * ###############################################################
 * Simple Tests macros
 * ###############################################################
 */

#define ST_TEST(TESTNAME, DESCRIPTION) \
    static void TESTNAME(); \
    static SimpleTests::AutoRegister autoRegister_##TESTNAME(#TESTNAME, DESCRIPTION, TESTNAME);\
    static void TESTNAME()

#define ST_TEST_ONLY(TESTNAME, DESCRIPTION) \
    static void TESTNAME(); \
    static SimpleTests::AutoRegister autoRegister_##TESTNAME(#TESTNAME, DESCRIPTION, TESTNAME, true);\
    static void TESTNAME()

#define ST_ASSERT(EXPRESSION, DESCRIPTION) \
    if (!(EXPRESSION)) \
    {\
        std::stringstream ss;\
        ss << "Failed: " << DESCRIPTION << " (" << __FILE__ << ":" << __LINE__ << ")\n";\
        SimpleTests::SimpleTestsContext::notifyAssertFailed(ss.str());\
        return;\
    }

#define ST_SECTION_BEGIN(SECTIONNAME, DESCRIPTION) \
    static SimpleTests::SectionBeginner sectionBeginner_##SECTIONNAME(#SECTIONNAME, DESCRIPTION);\
    namespace

#define ST_SECTION_BEGIN_ONLY(SECTIONNAME, DESCRIPTION) \
    static SimpleTests::SectionBeginner sectionBeginner_##SECTIONNAME(#SECTIONNAME, DESCRIPTION, true);\
    namespace

#define ST_SECTION_END(SECTIONNAME) \
    static SimpleTests::SectionEnder sectionEnder_##SECTIONNAME;
    
#define ST_BEFORE_EACH(FUNCTORNAME) \
    static void FUNCTORNAME(); \
    static SimpleTests::SectionBeforeEachRegister beforeEach_##FUNCTORNAME(FUNCTORNAME); \
    static void FUNCTORNAME()

#define ST_AFTER_EACH(FUNCTORNAME) \
    static void FUNCTORNAME(); \
    static SimpleTests::SectionAfterEachRegister afterEach_##FUNCTORNAME(FUNCTORNAME); \
    static void FUNCTORNAME()
