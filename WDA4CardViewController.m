//
//  WDA4CardViewController.m
//  icc
//
//  Created by YSHI_LGQ on 2017/12/1.
//

#import "WDA4CardViewController.h"
#import "CardIOView.h"
#import "CardIOCreditCardInfo.h"

typedef NS_ENUM(NSInteger ,WDCardType) {
  WDCardTypeFront = 1001,
  WDCardTypeBack = 1002,
};

@interface WDA4CardViewController ()<CardIOViewDelegate>
@property (nonatomic, strong) UIImage *imageFront;
@property (nonatomic, strong) UIImage *imageBack;

@property (nonatomic, strong) UIImageView *resultImageView;
@property (nonatomic, strong) UILabel *labDesc;

@property (nonatomic, assign) WDCardType currentType;

@property (strong, nonatomic) CardIOView *previewView;

@end


@implementation WDA4CardViewController

- (void)viewDidLoad {
    [super viewDidLoad];
  
    UIButton *btnStart = [UIButton buttonWithType:UIButtonTypeCustom];
    [btnStart setTitle:@"证件正面" forState:UIControlStateNormal];
    [btnStart setBackgroundColor:[UIColor blueColor]];
  [btnStart addTarget:self action:@selector(beginAction:) forControlEvents:UIControlEventTouchUpInside];
    btnStart.frame = CGRectMake(15, 40, (self.view.bounds.size.width - 60) * 0.5, 40);
    [self.view addSubview:btnStart];
  
    UIButton *btnStart1 = [UIButton buttonWithType:UIButtonTypeCustom];
    [btnStart1 setTitle:@"证件背面" forState:UIControlStateNormal];
    [btnStart1 setBackgroundColor:[UIColor blueColor]];
  [btnStart1 addTarget:self action:@selector(beginAction:) forControlEvents:UIControlEventTouchUpInside];
    btnStart1.frame = CGRectMake(self.view.bounds.size.width * 0.5 + 15, 40, (self.view.bounds.size.width - 60) * 0.5, 40);
    [self.view addSubview:btnStart1];

    btnStart.tag = WDCardTypeFront;
    btnStart1.tag = WDCardTypeBack;
  
    [self.view addSubview:self.previewView];
    [self.view addSubview:self.resultImageView];
  
  UIButton *btnSave = [UIButton buttonWithType:UIButtonTypeCustom];
  [btnSave setTitle:@"保存成图片" forState:UIControlStateNormal];
  [btnSave setBackgroundColor:[UIColor blueColor]];
  [btnSave addTarget:self action:@selector(saveAction) forControlEvents:UIControlEventTouchUpInside];
  btnSave.frame = CGRectMake(0, self.view.bounds.size.height-50, self.view.bounds.size.width, 50);
  [self.view addSubview:btnSave];
  
  [self.view addSubview:self.labDesc];
}

- (void)saveAction
{
    if (self.resultImageView.image) {
        [self saveImageToPhotos:self.resultImageView.image];
    }else {
      _labDesc.text = @"请先拍摄证件正反面";
    }
}

- (void)beginAction:(UIButton *)btn
{
    self.resultImageView.hidden = YES;
    _currentType = btn.tag;
   //开始取卡片
    self.previewView.hidden = !self.previewView.hidden;
}


- (void)saveImageToPhotos:(UIImage*)savedImage
{
  UIImageWriteToSavedPhotosAlbum(savedImage, self, @selector(image:didFinishSavingWithError:contextInfo:), NULL);

}

//回调方法
- (void)image: (UIImage *) image didFinishSavingWithError: (NSError *) error contextInfo: (void *) contextInfo
{
  NSString *msg = nil ;
  if(error != NULL){
    msg = @"保存图片失败" ;
  }else{
    msg = @"保存图片成功" ;
  }
  _labDesc.text = msg;

}
#pragma mark - dataSource && delegate
- (void)cardIOView:(CardIOView *)cardIOView didScanCard:(CardIOCreditCardInfo *)cardInfo
{
  
    CGFloat x = 38;
    CGFloat w = self.view.bounds.size.width - 2 * x;
  
    if (_currentType == WDCardTypeFront) {
      _imageFront = cardInfo.cardImage;
      _labDesc.text = @"请拍摄证件背面";
      self.resultImageView.image = _imageFront;
      CGFloat h = w * _imageFront.size.height / _imageFront.size.width;
      _resultImageView.frame = CGRectMake(x, 120, w, h);

    }
    else {
      _imageBack = cardInfo.cardImage;
    }
  
    if (_imageFront && _imageBack) {
      _labDesc.text = @"";
      self.resultImageView.image = [self addImage:_imageFront withImage:_imageBack];
      _imageFront = nil;
      _imageBack = nil;
 
      CGFloat h = w * _resultImageView.image.size.height / _resultImageView.image.size.width;
      
      _resultImageView.frame = CGRectMake(x, 120, w, h);
      
    }
  
    dispatch_async(dispatch_get_main_queue(), ^{
//        [self.previewView removeFromSuperview];
      self.previewView.hidden = YES;
    });

  self.resultImageView.hidden = NO;

}

- (UIImage *)addImage:(UIImage *)imageFront withImage:(UIImage *)imageBack {
  
    UIImage *imageBackground = [UIImage imageNamed:@"A4"];
  
    UIGraphicsBeginImageContext(imageBackground.size);
  
    [imageFront drawInRect:CGRectMake(0, 0, imageFront.size.width, imageFront.size.height)];
  
    CGFloat w = 85.6/206 * imageBackground.size.width;
    CGFloat h = w * 54/86.5;
  
    CGFloat x = (imageBackground.size.width - w) * 0.5;
    CGFloat y = imageBackground.size.height * 0.5 - 2 * h;
  
    CGRect frontRect = CGRectMake(x, y, w, h);
    CGRect backRect = CGRectMake(x, imageBackground.size.height * 0.5 + h, w, h);
  
    [imageBackground drawInRect:CGRectMake(0, 0, imageBackground.size.width, imageBackground.size.height)];
  
    [imageFront drawInRect:frontRect];
    [imageBack drawInRect:backRect];
  
    UIImage *resultingImage = UIGraphicsGetImageFromCurrentImageContext();
  
    UIGraphicsEndImageContext();
  
    return resultingImage;
}


#pragma mark lazy

- (CardIOView *)previewView
{
  if (!_previewView) {
    _previewView = [[CardIOView alloc] init];
    _previewView.guideColor = [UIColor cyanColor];
    _previewView.hideCardIOLogo = YES;
    //CardIODetectionModeCardImageOnly CardIODetectionModeAutomatic
    _previewView.detectionMode = CardIODetectionModeCardImageOnly;
    _previewView.scanExpiry = NO;
    _previewView.languageOrLocale = @"zh-Hans";
    _previewView.delegate = self;
    _previewView.hidden = YES;
    _previewView.frame = CGRectMake(0, 120, self.view.bounds.size.width, self.view.bounds.size.width);
  }
  return _previewView;
}


- (UIImageView *)resultImageView
{
  
  if (!_resultImageView) {
    _resultImageView = [[UIImageView alloc] init];
    _resultImageView.layer.borderColor = [UIColor grayColor].CGColor;
    _resultImageView.layer.borderWidth = 2;
  }
  return _resultImageView;
}

- (UILabel *)labDesc
{
  if (!_labDesc) {
    _labDesc = [[UILabel alloc] initWithFrame:CGRectMake(0, 96, self.view.bounds.size.width, 30)];
    _labDesc.font = [UIFont systemFontOfSize:18];
    _labDesc.textColor = [UIColor redColor];
    _labDesc.textAlignment = NSTextAlignmentCenter;
    _labDesc.numberOfLines = 0;
    _labDesc.text = @"请拍摄证件正面";
  }
  return _labDesc;
}

@end
