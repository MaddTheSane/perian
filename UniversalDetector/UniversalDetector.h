#import <Cocoa/Cocoa.h>

#define UniversalDetector SubUniversalDetector

@interface UniversalDetector : NSObject
{
	void *detectorptr;
	NSString *charset;
	float confidence;
}
@property (retain, nonatomic, readonly) NSString *MIMECharset;
@property (readonly) NSStringEncoding encoding;
@property (readonly) float confidence;

- (id)init;

- (void)analyzeData:(NSData *)data;
- (void)analyzeBytes:(const char *)data length:(int)len;
- (void)reset;

- (BOOL)done DEPRECATED_ATTRIBUTE;
- (BOOL)isDone;

- (void)debugDump;

+ (UniversalDetector *)detector;

@end
