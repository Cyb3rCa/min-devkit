/// @file	
///	@ingroup 	minexamples
///	@copyright	Copyright (c) 2016, Cycling '74
/// @author		Timothy Place
///	@license	Usage of this file and its contents is governed by the MIT License

#include "c74_min.h"
#include <queue>

using namespace c74::min;


using pitch = int;
using velocity = int;
using duration = int;


// forward declaration of the note class so we can use it in the definition of `note`

class note_make;


// the note class represents a single note generated by the note_make class
// here we just create the class interface...
// the implementation will be later so that we have access to e.g. the outlets of note_make

class note {
public:
	note(note_make* owner, pitch, duration);

	long id() { return m_id; }

private:
	note_make*			m_owner;	// we need to know who our owner is for the timer setup and the outlet calls
	pitch				m_pitch;	// pitch we keep for the noteoff, we don't need velocity
	c74::min::function	m_off_fn;	// note-off function callback for the timer
	timer				m_timer;	// each note has its own timer to trigger the noteoff
	long				m_id;		// unique serial number for each note

	static long s_counter;
};


// We maintain our collection of active notes in a list.
// We cannot use a FIFO queue because the duration of the notes may all be independent.
//
// Instead of keeping copies of the notes we use references, actually we use unique_ptr...
// We do this (avoid copies) because the note contains a timer and we cannot make copies of timers.

using notes = std::list< std::unique_ptr<note> >;


// Finally, our Max class ...

class note_make : public object<note_make> {
public:
	friend class note;

	MIN_DESCRIPTION { "Generate a note-on/note-off pair. Just like the makenote object." };
	MIN_TAGS		{ "midi, time" };
	MIN_AUTHOR		{ "Cycling '74" };
	MIN_RELATED		{ "makenote" };
	
	inlet<>		pitch_in		{ this, "(int) pitch" };
	inlet<>		velocity_in		{ this, "(int) velocity" };
	inlet<>		duration_in		{ this, "(int) duration" };

	outlet<>	pitch_out		{ this, "(int) pitch" };
	outlet<>	velocity_out	{ this, "(int) velocity" };

	argument<number> velocity_arg	{ this, "velocity", "Initial MIDI velocity.",
		MIN_ARGUMENT_FUNCTION {
			m_velocity = arg;
		}
	};

	argument<number> duration_arg	{ this, "duration", "Initial duration in milliseconds.",
		MIN_ARGUMENT_FUNCTION {
			m_duration = arg;
		}
	};

	// the truth is that this only sort-of threadsafe
	// when receiving some bits of info from the scheduler and some from the main thread
	// it won't crash or do anything catastrophic
	// however, the wrong velocities might be paired with the wrong pitches, etc.

	message<threadsafe::yes> ints { this, "int", "MIDI note information",
		MIN_FUNCTION {
			switch (inlet) {
				case 0:
					m_pitch = args[0];
					start();
					break;
				case 1:
					m_velocity = args[0];
					break;
				case 2:
					m_duration = args[0];
					break;
				default:
					assert(false);
			}
			return {};
		}
	};

private:
	notes		m_notes;
	pitch		m_pitch;
	velocity	m_velocity;
	duration	m_duration;

	void start() {
		velocity_out.send(m_velocity);
		pitch_out.send(m_pitch);
		m_notes.push_back( std::make_unique<note>(this, m_pitch, m_duration) );
	}

	void remove(long note_id) {
		bool note_removed = false;

		for (auto iter = m_notes.begin(); iter != m_notes.end(); ++iter) {
			const auto& stored_note = (*iter).get();

			if (stored_note->id() == note_id) {
				m_notes.erase(iter);
				note_removed = true;
				break;
			}
		}
		assert(note_removed); // post-condition: if a note wasn't removed we have serious problems.
	}
};


// The implementation of the constructor for the note class ...
// The order of initialization is critical

note::note(note_make* owner, pitch a_pitch, duration a_duration)
: m_owner { owner }
, m_pitch { a_pitch }
, m_off_fn { MIN_FUNCTION {
	m_owner->velocity_out.send(0);
	m_owner->pitch_out.send(m_pitch);
	m_owner->remove(m_id);
	return {};}
  }
, m_timer { m_owner, m_off_fn }
, m_id { ++s_counter }
{
	m_timer.delay(a_duration);
}

long note::s_counter = 0;


MIN_EXTERNAL(note_make);
