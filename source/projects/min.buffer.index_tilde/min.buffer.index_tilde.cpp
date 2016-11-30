/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"

using namespace c74::min;

class buffer_index : public object<buffer_index>, perform_operator {
public:
	
	MIN_DESCRIPTION { "Read from a buffer~." };
	MIN_TAGS		{ "audio, sampling" };
	MIN_AUTHOR		{ "Cycling '74" };
	MIN_RELATED		{ "index~, buffer~, wave~" };

	inlet<>				index_inlet		{ this, "(signal) Sample index" };
	inlet<>				channel_inlet	{ this, "(float) Audio channel to use from buffer~" };
	outlet<>			output			{ this, "(signal) Sample value at index", "signal" };
	outlet<>			changed			{ this, "(symbol) Notification that the content of the buffer~ changed." };

	buffer_reference	buffer			{ this, 
		MIN_FUNCTION {
			// will receive a symbol arg indicating 'binding', 'unbinding', or 'modified'
			changed.send(args);
			return {};
		}
	};

	
	argument<symbol> name_arg { this, "buffer-name", "Initial buffer~ from which to read.",
		MIN_ARGUMENT_FUNCTION {
			buffer.set(arg);
		}
	};

	argument<int> channel_arg { this, "channel", "Initial channel to read from the buffer~.",
		MIN_ARGUMENT_FUNCTION {
			channel = arg;
		}
	};


	attribute<int> channel { this, "channel", 1,
		description { "Channel to read from the buffer~." },
		setter { MIN_FUNCTION {
			int n = args[0];
			if (n < 1)
				n = 1;
			return {n};
		}}
	};


	void perform(audio_bundle input, audio_bundle output) {
		auto			in = input.samples(0);
		auto			out = output.samples(0);
		buffer_lock<>	b(buffer);
		auto			chan = std::min<int>(channel-1, b.channelcount());
		
		if (b.valid()) {
			for (auto i=0; i<input.framecount(); ++i) {
				auto frame = size_t(in[i] + 0.5);
				out[i] = b.lookup(frame, chan);
			}
		}
		else {
			for (auto i=0; i<input.framecount(); ++i)
				out[i] = 0.0;
		}
	}
	
};


MIN_EXTERNAL(buffer_index);
