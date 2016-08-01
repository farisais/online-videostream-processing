/*
 *  Video Decoder & Encoder Interface in Go. Will call the C
 *  Av library using CGO
 *
 *  Copyright (c) 2016 - Faris Rahman <farisais@hotmail.com>
 *
 *  This source code is part of the tutorial
 *  in https://github.com/farisais/online-videostream-processing
 */

package main

/*
#cgo CFLAGS: -I ../av
#cgo LDFLAGS: -L${SRCDIR} -lavlib -lavformat -lavcodec -lx264 -lavdevice -lavutil -lswscale -lswresample -lz -lm
#include "av.h"
void avDecodeCallback(char* data, int size, char* secret);
*/
import "C"
import "unsafe"

//export avDecodeCallback
func avDecodeCallback(data *C.char, size C.int, secret *C.char) {
	decoded_result := C.GoBytes(unsafe.Pointer(data),
		C.int(size))
	secret_c := C.GoString(secret)
	for _, con := range InConnections {
		if con.session == secret_c {
			con.avInterface.PipeOutDecoder <- decoded_result
		}
	}
}

type AvProcessInterface struct {
	PipeOutEncoder chan []byte
	PipeOutDecoder chan []byte
}

func NewAvInterface(codec *string, secret string) *AvProcessInterface {
	avint := &AvProcessInterface{
		PipeOutEncoder: make(chan []byte, 100),
		PipeOutDecoder: make(chan []byte, 100),
	}

	cargs := C.CString(*codec)
	csecret := C.CString(secret)
	C.init_decoder(cargs, csecret)
	C.init_encoder()

	C.free(unsafe.Pointer(cargs))
	C.free(unsafe.Pointer(csecret))
	return avint
}

func (avint *AvProcessInterface) RunEncoder(PipeIn *chan []byte) {
	for d := range *PipeIn {
		cargs := C.CString(string(d))
		res_ptr := C.encode_video(cargs, C.size_t(len(d)))
		enc_struct := (*C.struct_EncodeResult)(unsafe.Pointer(res_ptr))
		/*
		 * Check if we got the result
		 */
		if enc_struct.encoded == 1 {
			/*
			 * Get the C data pointer within the struct
			 */
			enc_frame := C.GoBytes(unsafe.Pointer(enc_struct.data), C.int(enc_struct.size))
			avint.PipeOutEncoder <- enc_frame
		}
		/*
		 * We need to free the C allocated pointer as it won't be freed by Go garbage collector
		 */
		C.free(unsafe.Pointer(cargs))
	}
}

func (avint *AvProcessInterface) RunDecoder(PipeIn *chan []byte) {
	for d := range *PipeIn {
		cargs := C.CString(string(d))
		C.decode_video2(cargs, C.size_t(len(d)))
		//dec_struct := (*C.struct_DecodeResult)(unsafe.Pointer(res_ptr))
		/*
		 * Check if we got result
		 */
		//if dec_struct.got_picture == 1 {
		/*
		 * Get the decoded frame from the buffer
		 */
		// dec_frame := C.GoBytes(unsafe.Pointer(dec_struct.ppm_frame_buffer),
		// 	C.int(dec_struct.ppm_frame_size))
		// avint.PipeOutDecoder <- dec_frame
		//}
		/*
		 * We need to free the C allocated pointer as it won't be freed by Go garbage collector
		 */
		C.free(unsafe.Pointer(cargs))
	}
}
