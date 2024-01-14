#pragma once

#include "loop.hpp"
#include "trace.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <mutex>

namespace pcs {
    std::mutex ungenerated_trace_mtx;
    std::mutex trace_mtx;

    void path_thread(std::vector<Loop> *loops, std::vector<Trace> *ungenerated_traces, std::vector<Trace> *traces) {
        while(true) {
            ungenerated_trace_mtx.lock();
            if(ungenerated_traces->size() == 0) {
                std::cout << "end" << std::endl;
                ungenerated_trace_mtx.unlock();
                return;
            }
            Trace t = ungenerated_traces->back();
            ungenerated_traces->pop_back();
            ungenerated_trace_mtx.unlock();
            
            t.generate_dynamic_path(loops, 1000);

            trace_mtx.lock();
            traces->push_back(t);
            trace_mtx.unlock();
        }
    }

    void compile_em(std::vector<Loop> *loops, std::vector<Trace> *traces) {
        int num_loops = 12;
        for(int i = 0; i < num_loops; i++) {
            loops->push_back(Loop::create_sine_circle(glm::vec3(cos(two_pi<float>() * i / num_loops), 0, -sin(two_pi<float>() * i / num_loops)) * 1.4f, glm::vec3(sin(two_pi<float>() * i / num_loops), 0, cos(two_pi<float>() * i / num_loops)), 1.0, 1.0, 100));
        }
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));
        loops->push_back(Loop::create_circle(glm::vec3(0.0f, 0.4f, 0.0f), glm::vec3(0, 1, 0), 0.3, 4.0, 100));

        int levels = 6;
        int samples = 16;
        
        std::vector<Trace> ungenerated_traces;

        for(int r = 0; r < levels; r++) {
            for(int i = 0; i < samples; i++) {
                Trace t = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0+1.4, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), true);
                Trace t2 = Trace(glm::vec3(glm::cos(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0+1.4, glm::sin(glm::two_pi<float>() * i / samples)*(((float)r+0.5)/(float)(levels+1))*1.0, 0.0), false);
                // t.generate_path(loops, 0.005, 1000, true);
                // t2.generate_path(loops, 0.005, 1000, false);
                // t.generate_dynamic_path(loops, 1000, true);
                // t2.generate_dynamic_path(loops, 1000, false);
                // traces->push_back(t);
                // traces->push_back(t2);
                ungenerated_traces.push_back(t);
                ungenerated_traces.push_back(t2);
            }
        }

        int conc = std::thread::hardware_concurrency();
        std::cout << "threads: " << conc << std::endl;

        std::thread ts[conc-1];
        for(int i = 0; i < conc - 1; i++) {
            ts[i] = std::thread(path_thread, loops, &ungenerated_traces, traces);
        }
        std::cout << "before main thread path calcs" << std::endl;
        path_thread(loops, &ungenerated_traces, traces);
        std::cout << "after main thread path calcs" << std::endl;
        for(int i = 0; i < conc - 1; i++) {
            ts[i].join();
        }
        std::cout << "after joins" << std::endl;
    }
};
