#import <Cocoa/Cocoa.h>

#define UniversalDetector SubUniversalDetector

@interface UniversalDetector : NSObject
{
	void *detectorptr;
	NSString *charset;
	float confidence;
}
@property (retain, nonatomic, readonly) NSString *MIMECharset;

-(id)init;

-(void)analyzeData:(NSData *)data;
-(void)analyzeBytes:(const char *)data length:(int)len;
-(void)reset;

-(BOOL)done;
-(NSString *)MIMECharset;
-(NSStringEncoding)encoding;
-(float)confidence;

-(void)debugDump;

+(UniversalDetector *)detector;

@end
