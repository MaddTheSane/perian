/*
 * ECQTComponent.m
 * Copyright (c) 2006 Ken Aspeslagh
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
//
//  ECQTComponent.h
//  QTManager
//
//  Created by Ken on 12/15/07.
//

#import <Cocoa/Cocoa.h>

typedef enum _componentScope
{
	componentIsLocal = 0,
	componentIsGlobal = 1,
	componentIsSystem = 2
	
} componentScope;


@interface ECQTComponent : NSObject 
{
	componentScope mWhere;

	NSMutableDictionary* mParameters;
}

+(id)componentWithPath:(NSString*)path;

-(void)setComponentInfo;
-(void)setComponentScope;

-(componentScope)isWhere;

-(BOOL)extended;
-(void)setEnabled:(BOOL)enabled;

-(NSString*)name;
-(NSString*)location;
-(NSString*)version;
-(NSString*)type;
-(NSNumber*)compVersion;
-(NSNumber*)compFlags;
-(NSString*)arches;
-(NSString*)whereSort;
-(NSNumber*)componentEnabled;

-(NSString*)types;
-(NSString*)names;


@end
