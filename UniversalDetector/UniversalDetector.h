#import <Cocoa/Cocoa.h>

#define UniversalDetector SubUniversalDetector

@interface UniversalDetector : NSObject
{
	void *detectorptr;
	NSString *charset;
	float confidence;
}
@property (copy, nonatomic, readonly) NSString *MIMECharset;
@property (readonly) NSStringEncoding encoding;
@property (readonly) float confidence;

- (instancetype)init;

- (void)analyzeData:(NSData *)data;
- (void)analyzeBytes:(const char *)data length:(int)len;
- (void)reset;

- (BOOL)done DEPRECATED_ATTRIBUTE;
@property (readonly, getter=isDone) BOOL done;

- (void)debugDump;

+ (instancetype)detector;

@end
