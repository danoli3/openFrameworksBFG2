#include "ofxThreadedImageLoader.h"
#include <sstream>
ofxThreadedImageLoader::ofxThreadedImageLoader() 
:ofThread()
{
	num_loading = 0;
    ofAddListener(ofEvents().update, this, &ofxThreadedImageLoader::update);
	ofRegisterURLNotification(this);    
    
    startThread();
    lastUpdate = 0;
    errorCounter = 0;
}

// Load an image from disk.
//--------------------------------------------------------------
void ofxThreadedImageLoader::loadFromDisk(ofImage* image, string filename) {
    
	
	num_loading++;
	ofImageLoaderEntry entry(image, OF_LOAD_FROM_DISK);
	entry.filename = filename;
	entry.id = num_loading;
	entry.image->setUseTexture(false);
	entry.name = filename;
    
    lock();
    images_to_load_buffer.push_back(entry);
    condition.signal();
    unlock();
    
    
}


// Load an url asynchronously from an url.
// * 5 bucks this doesn't work
//--------------------------------------------------------------
void ofxThreadedImageLoader::loadFromURL(ofImage* image, string url) {

	
	num_loading++;
	ofImageLoaderEntry entry(image, OF_LOAD_FROM_URL);
	entry.url = url;
	entry.id = num_loading;
	entry.image->setUseTexture(false);	
	
	stringstream ss;
	ss << "image" << entry.id;
	entry.name = ss.str();

    lock();
	images_to_load_buffer.push_back(entry);
    condition.signal();
    unlock();
}


// Reads from the queue and loads new images.
//--------------------------------------------------------------
void ofxThreadedImageLoader::threadedFunction() {
    
    errorCounter = 0;
    
	while( isThreadRunning() ) {
        
        try {
            
            lock();
            // wake every 1/2 second just in case we missed something
            condition.tryWait(this->mutex, 500);

            images_to_load.insert( images_to_load.end(),
                                   images_to_load_buffer.begin(),
                                   images_to_load_buffer.end() );
            
            images_to_load_buffer.clear();
            unlock();
            
            while( shouldLoadImages() ) {
                ofImageLoaderEntry entry = getNextImageToLoad();
                if(entry.image == NULL) {
                    continue;
                }
            
                if(entry.type == OF_LOAD_FROM_DISK) {
                    if(! entry.image->loadImage(entry.filename) )  { 
                        stringstream ss;
                        ss << "ofxThreadedImageLoader error loading image " << entry.filename;
                        ofLog(OF_LOG_ERROR, ss.str() );
                    }
                    
                    images_to_update.push_back(entry);
                    //cout << "loaded from disk " << entry.name << endl;
                }
                else if(entry.type == OF_LOAD_FROM_URL) {
                    images_async_loading.push_back(entry);
                    ofLoadURLAsync(entry.url, entry.name);
                }
                
                // also check while looping
                lock();
                images_to_load.insert( images_to_load.end(),
                                       images_to_load_buffer.begin(),
                                       images_to_load_buffer.end() );
                
                images_to_load_buffer.clear();
                unlock();

            }
            
            errorCounter = 0;
            
        } catch (exception& e) {
            stringstream ss;
            ss << "Exception caught in ofxThreadedImageLoader: " << e.what() << endl;
            ofLog( OF_LOG_ERROR, ss.str() );

            ++errorCounter;
            // don't overload log
            sleep(errorCounter * 1000);
            
            unlock();
        }
	}
}


// When we receive an url response this method is called; 
// The loaded image is removed from the async_queue and added to the
// update queue. The update queue is used to update the texture.
//--------------------------------------------------------------
void ofxThreadedImageLoader::urlResponse(ofHttpResponse & response) {
	if(response.status == 200) {
        
		lock();
		
		// Get the loaded url from the async queue and move it into the update queue.
		entry_iterator it = getEntryFromAsyncQueue(response.request.name);
		if(it != images_async_loading.end()) {
			(*it).image->loadImage(response.data);
            
			images_async_loading.erase(it);
            //cout << "loaded " << it->name << endl;
		}
		
		unlock();
	}
	else {
        
        lock();
        
		// log error.
		stringstream ss;
		ss << "Could not image from url, response status: " << response.status;
		ofLog(OF_LOG_ERROR, ss.str());
		
		// remove the entry from the queue
		lock();
		entry_iterator it = getEntryFromAsyncQueue(response.request.name);
		if(it != images_async_loading.end()) {
			images_async_loading.erase(it);
		}
		else {
			ofLog(OF_LOG_WARNING, "Cannot find image in load-from-url queue");
		}
		unlock();
	}
}


// Check the update queue and update the texture
//--------------------------------------------------------------
void ofxThreadedImageLoader::update(ofEventArgs & a){
    
    // TODO put a max on the number of images we copy over
    // so that we don't slow thngs down
    
    lock();
	ofImageLoaderEntry entry = getNextImageToUpdate();
	while (entry.image != NULL) {

		const ofPixels& pix = entry.image->getPixelsRef();
		entry.image->getTextureReference().allocate(
				 pix.getWidth()
				,pix.getHeight()
				,ofGetGlInternalFormat(pix)
		);
		
		entry.image->setUseTexture(true);
		entry.image->update();
        
        //cout << "Updated " << entry.name << endl;
        entry = getNextImageToUpdate();
	}
    unlock();
}


// Find an entry in the aysnc queue.
//   * private, no lock protection, is private function
//--------------------------------------------------------------
ofxThreadedImageLoader::entry_iterator ofxThreadedImageLoader::getEntryFromAsyncQueue(string name) {
	entry_iterator it = images_async_loading.begin();
	while(it != images_async_loading.end()) {
		if((*it).name == name) {
			return it;
		}
	}
	return images_async_loading.end();
}


// Pick an entry from the queue with images for which the texture
// needs to be update.
//   * private, no lock protection, is private function
//--------------------------------------------------------------
ofxThreadedImageLoader::ofImageLoaderEntry ofxThreadedImageLoader::getNextImageToUpdate() {
    
	ofImageLoaderEntry entry;
	if(images_to_update.size() > 0) {
		entry = images_to_update.front();
		images_to_update.pop_front();
	}
	return entry;
}

// Pick the next image to load from disk.
//   * private, no lock protection, is private function
//--------------------------------------------------------------
ofxThreadedImageLoader::ofImageLoaderEntry ofxThreadedImageLoader::getNextImageToLoad() {
	
    ofImageLoaderEntry entry;
	if(images_to_load.size() > 0) {
		entry = images_to_load.front();
		images_to_load.pop_front();
	}
	return entry;
}

// Check if there are still images in the queue.
//   * private, no lock protection, is private function
//--------------------------------------------------------------
bool ofxThreadedImageLoader::shouldLoadImages() {
    bool temp = images_to_load.size() > 0;
    
	return temp;
}
