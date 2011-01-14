#pragma once

//----------------------------------------------------------
// ofColor
//----------------------------------------------------------

class ofColor{
	public:
	
		ofColor ();
		virtual ~ ofColor ();
		
		ofColor (float _r, float _g, float _b, float _a = 255.0f);
		ofColor (ofColor const & color);
		ofColor (ofColor const & color, float _a);
		ofColor (float gray, float _a = 255.0f);
		
		static ofColor fromHsb (float hue, float saturation, float brightness, float alpha = 255.f);
		static ofColor fromHex (int hexColor, float alpha = 255.f);
		
		static ofColor gray;
		static ofColor white;
		static ofColor red;
		static ofColor green;
		static ofColor blue;
		static ofColor black;
	
	void set (float _r, float _g, float _b, float _a = 255.0f);
		void set (float _gray, float _a = 255.0f);
		void set (ofColor const & color);

		void setHex (int hexColor, float alpha = 255.0f);
		int getHex () const;
		
		ofColor& clamp ();
		ofColor& invert ();
		ofColor& normalize ();
		ofColor& lerp(const ofColor& target, float amount);
		
		ofColor getClamped () const;
		ofColor getInverted () const;
		ofColor getNormalized () const;
		ofColor getLerped(const ofColor& target, float amount) const;
		
		float getHue () const;
		float getSaturation () const;
		float getBrightness () const; // brightest component
		float getLightness () const; // average of the components
		void getHsb(float& hue, float& saturation, float& brightness) const;
		
		void setHue (float hue);
		void setSaturation (float saturation);
		void setBrightness (float brightness);
		void setHsb(float hue, float saturation, float brightness, float alpha);
		void setHsb(float hue, float saturation, float brightness);
		
		ofColor & operator = (ofColor const & color);
		ofColor & operator = (float const & val);
		bool operator == (ofColor const & color);
		bool operator != (ofColor const & color);
		ofColor operator + (ofColor const & color) const;
		ofColor operator + (float const & val) const;
		ofColor & operator += (ofColor const & color);
		ofColor & operator += (float const & val);
		ofColor operator - (ofColor const & color) const;
		ofColor operator - (float const & val) const;
		ofColor & operator -= (ofColor const & color);
		ofColor & operator -= (float const & val);
		ofColor operator * (ofColor const & color) const;
		ofColor operator * (float const & val) const;
		ofColor & operator *= (ofColor const & color);
		ofColor & operator *= (float const & val);
		ofColor operator / (ofColor const & color) const;
		ofColor operator / (float const & val) const;
		ofColor & operator /= (ofColor const & color);
		ofColor & operator /= (float const & val);
		float & operator [] (int n);
		
		union  {
			struct {
				float r, g, b, a;
			};
			float v[4];
		};


};

