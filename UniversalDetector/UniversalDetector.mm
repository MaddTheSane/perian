#define uint32 CSSM_uint32
#import "UniversalDetector.h"
#undef uint32

#import "nscore.h"
#import "nsUniversalDetector.h"
#import "nsCharSetProber.h"

class wrappedUniversalDetector : public nsUniversalDetector
{
public:
	void Report(const char* aCharset) {}
	
	const char *charset(float &confidence)
	{
		if(!mGotData)
		{
			confidence=0;
			return 0;
		}
		
		if(mDetectedCharset)
		{
			confidence=1;
			return mDetectedCharset;
		}
		
		switch(mInputState)
		{
			case eHighbyte:
			{
				float proberConfidence;
				float maxProberConfidence = (float)0.0;
				PRInt32 maxProber = 0;
				
				for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
				{
					proberConfidence = mCharSetProbers[i]->GetConfidence();
					if (proberConfidence > maxProberConfidence)
					{
						maxProberConfidence = proberConfidence;
						maxProber = i;
					}
				}
				
				confidence=maxProberConfidence;
				return mCharSetProbers[maxProber]->GetCharSetName();
			}
				break;
				
			default:
			case ePureAscii:
				confidence=0;
				return "US-ASCII";
				break;
		}
		
		confidence=0;
		return 0;
	}
	
	bool done()
	{
		if(mDetectedCharset) return true;
		return false;
	}
    
    void debug()
    {
        for (PRInt32 i = 0; i < NUM_OF_CHARSET_PROBERS; i++)
        {
            // If no data was received the array might stay filled with nulls
            // the way it was initialized in the constructor.
            if (mCharSetProbers[i])
                mCharSetProbers[i]->DumpStatus();
        }
    }
	
	void reset() { Reset(); }
};

@interface UniversalDetector ()
@property (retain, nonatomic, readwrite) NSString *MIMECharset;
@end

@implementation UniversalDetector
@synthesize MIMECharset = charset;
@synthesize confidence;
@dynamic encoding;

- (id)init
{
	if (self = [super init]) {
		detectorptr=(void *)new wrappedUniversalDetector;
	}
	return self;
}

- (void)dealloc
{
	delete (wrappedUniversalDetector *)detectorptr;
	self.MIMECharset = nil;
	[super dealloc];
}

- (void)analyzeData:(NSData *)data
{
	[self analyzeBytes:(const char *)[data bytes] length:[data length]];
}

- (void)analyzeBytes:(const char *)data length:(int)len
{
	wrappedUniversalDetector *detector=(wrappedUniversalDetector *)detectorptr;

	if(detector->done()) return;

	detector->HandleData(data,len);
	self.MIMECharset = nil;
}

-(void)reset
{
	wrappedUniversalDetector *detector=(wrappedUniversalDetector *)detectorptr;
	detector->reset();
}

- (BOOL)isDone
{
	wrappedUniversalDetector *detector=(wrappedUniversalDetector *)detectorptr;
	return detector->done() ? YES : NO;
}

- (BOOL)done
{
	return [self isDone];
}

- (NSString *)MIMECharset
{
	if(!charset)
	{
		wrappedUniversalDetector *detector=(wrappedUniversalDetector *)detectorptr;
		const char *cstr = detector->charset(confidence);
		if(!cstr)
			return nil;
		self.MIMECharset = @(cstr);
	}
	return charset;
}

- (NSStringEncoding)encoding
{
	NSString *mimecharset = self.MIMECharset;
	if (!mimecharset)
		return 0;
	CFStringEncoding cfenc = CFStringConvertIANACharSetNameToEncoding((CFStringRef)mimecharset);
	if (cfenc == kCFStringEncodingInvalidId)
		return 0;
	return CFStringConvertEncodingToNSStringEncoding(cfenc);
}

- (float)confidence
{
	if(!charset)
		[self MIMECharset];
	return confidence;
}

- (void)debugDump
{
    wrappedUniversalDetector *detector=(wrappedUniversalDetector *)detectorptr;
    detector->debug();
}

+ (UniversalDetector *)detector
{
	return [[[UniversalDetector alloc] init] autorelease];
}

@end
