/**
 * @file
 * @brief Source file for VideoCacheThread class
 * @author Jonathan Thomas <jonathan@openshot.org>
 *
 * @section LICENSE
 *
 * Copyright (c) 2008-2014 OpenShot Studios, LLC
 * <http://www.openshotstudios.com/>. This file is part of
 * OpenShot Library (libopenshot), an open-source project dedicated to
 * delivering high quality video editing and animation solutions to the
 * world. For more information visit <http://www.openshot.org/>.
 *
 * OpenShot Library (libopenshot) is free software: you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * OpenShot Library (libopenshot) is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OpenShot Library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../include/Qt/VideoCacheThread.h"

namespace openshot
{
	class DemoThreadPoolJob  : public ThreadPoolJob
	{
		public:
			DemoThreadPoolJob()
				: ThreadPoolJob ("Demo Threadpool Job")
			{}

			ReaderBase *reader;
			int64_t frame_number;

			JobStatus runJob() override
			{
				try
				{
					if (reader) {
						//#pragma omp critical
						//std::cout << "DemoThreadPoolJob::runJob()::reader->GetFrame(" << frame_number << ")" << endl;
						// Force the frame to be generated
						reader->GetFrame(frame_number);
					}

				}
				catch (const OutOfBoundsFrame & e)
				{
					// Ignore out of bounds frame exceptions
				}

				return jobHasFinished;
			}

			void removedFromQueue()
			{
				// This is called to tell us that our job has been removed from the pool.
				// In this case there's no need to do anything here.
			}

		private:
			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoThreadPoolJob)
	};


	// Constructor
	VideoCacheThread::VideoCacheThread()
	: Thread("video-cache"), speed(1), is_playing(false), position(1)
	, reader(NULL), max_frames(OPEN_MP_NUM_PROCESSORS * 2), current_display_frame(1)
    {
    }

    // Destructor
	VideoCacheThread::~VideoCacheThread()
    {
    }

    // Get the currently playing frame number (if any)
    int64_t VideoCacheThread::getCurrentFramePosition()
    {
    	if (frame)
    		return frame->number;
    	else
    		return 0;
    }

    // Set the currently playing frame number (if any)
    void VideoCacheThread::setCurrentFramePosition(int64_t current_frame_number)
    {
    	current_display_frame = current_frame_number;
		if (current_display_frame > position) {
			resetPosition();
		}
    }

	// Reset the position to the current_display_frame
	void VideoCacheThread::resetPosition()
	{
		#pragma omp critical(ResetPosition)
		position = current_display_frame;
	}

	// Seek the reader to a particular frame number
	void VideoCacheThread::Seek(int64_t new_position)
	{
		position = new_position;
	}

	// Play the video
	void VideoCacheThread::Play() {
		// Start playing
		is_playing = true;
	}

	// Stop the audio
	void VideoCacheThread::Stop() {
		// Stop playing
		is_playing = false;
	}

    // Start the thread
    void VideoCacheThread::run()
    {
		omp_set_num_threads(OPEN_MP_NUM_PROCESSORS);
		omp_set_nested(true);

		while (!threadShouldExit() && is_playing) {

			// Cache frames before the other threads need them
			// Cache frames up to the max frames
			while ((position - current_display_frame) < max_frames)
			{
				// Create new job to cache a frame
				auto* newJob = new DemoThreadPoolJob();
				newJob->reader = reader;
				newJob->frame_number = position;
				pool.addJob(newJob, true);
				cout << "Cache frame " << position << " ( " << (position - current_display_frame) << ")" << endl;

				// Is cache position behind current display frame?
				if (position < current_display_frame) {
					// Jump ahead (we can't cache what has already happened)
					position = current_display_frame + 1;
					cout << "Reset cache position to frame " << position << endl;
				}

				// Increment position
				position++;
			}

			// Sleep for 1 frame length
			usleep(10 * 1000);
		}
		
		return;
    }
}
