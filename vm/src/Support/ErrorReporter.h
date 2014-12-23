/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SVM_ERRORREPORTER_H
#define SVM_ERRORREPORTER_H

namespace llvm {

    class Instruction;
    class Twine;
    class SDNode;
    class SelectionDAG;
    class Function;

    void report_fatal_error(const Instruction *I, const Twine &msg);
    void report_fatal_error(const Instruction *I, const char *msg);

    void report_fatal_error(const SDNode *N, SelectionDAG &DAG, const Twine &msg);
    void report_fatal_error(const SDNode *N, SelectionDAG &DAG, const char *msg);

    void report_warning(const Twine &msg);
    void report_warning(const char *msg);

    void report_warning(const Function *F, const Twine &msg);
    void report_warning(const Function *F, const char *msg);

} // end namespace

#endif
