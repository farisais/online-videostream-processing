/*
 *	Processing interface to call python from GO and running
 *	face detection using OpenCV in python
 *
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com>
 *
 *  This source code is part of the tutorial
 *  in https://github.com/farisais/online-videostream-processing
 */

package main

import (
	"fmt"
	"github.com/sbinet/go-python"
	"os"
	"path"
)

const (
	EXECPATH = "processing"
)

func init() {
	err := python.Initialize()
	if err != nil {
		panic(err.Error())
	}
}

func evalPython() {
	if python.PyErr_Occurred() != nil {
		fmt.Println("Error in python function ...")
		python.PyErr_Print()
	}
}

type ProcessInterface struct {
	PipeOut  chan []byte
	ExecFunc *python.PyObject
}

func NewProcInterface() *ProcessInterface {
	/*
	 * Load the python function to be called every receive new data
	 */
	wd, _ := os.Getwd()
	wdp := path.Join(wd, EXECPATH)

	lock.Lock()
	python.PyRun_SimpleString("import sys")
	python.PyRun_SimpleString("sys.path.append('" + wdp + "')")
	mod := python.PyImport_ImportModule("face")
	evalPython()
	execfunc := mod.GetAttrString("process")
	evalPython()
	lock.Unlock()

	pi := &ProcessInterface{
		PipeOut:  make(chan []byte, 100),
		ExecFunc: execfunc,
	}
	return pi
}

func (pi *ProcessInterface) RunProcessing(PipeIn *chan []byte) {
	for d := range *PipeIn {
		lock.Lock()
		/*
		 * Set param to be passed to python function
		 */
		param := python.PyTuple_New(1)
		pyData := python.PyByteArray_FromStringAndSize(
			string(d),
		)
		python.PyTuple_SET_ITEM(param, 0, pyData)
		result := pi.ExecFunc.CallObject(param)
		evalPython()
		if result != nil {
			processedData := python.PyByteArray_AsBytes(result)
			if len(processedData) > 0 {
				/*
				 * Send the result to pipe
				 */
				pi.PipeOut <- processedData
			}
		}
		// param.Clear()
		// pyData.Clear()
		lock.Unlock()
	}
}
