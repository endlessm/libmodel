const {DModel} = imports.gi;

const InstanceOfMatcher = imports.tests.InstanceOfMatcher;

const MOCK_VIDEO_DATA = {
    '@id': 'ekn:///78901234567890123456',
    'title': 'Never Gonna Give You Up (Never Gonna Let You Down)',
    'caption': 'If this song was sushi, it would be a Rick Roll',
    'transcript': 'We\'re no strangers to love, etc etc etc',
    'duration': '666',
    'height': '666',
    'width': '666',
    'poster': 'ekn:///89012345678901234567',
};

describe('Video Object Model', function () {
    let videoObject;

    beforeEach(function () {
        jasmine.addMatchers(InstanceOfMatcher.customMatchers);
        videoObject = DModel.Video.new_from_json(MOCK_VIDEO_DATA);
    });

    describe('type', function () {
        it('should be a DModel.Media', function () {
            expect(videoObject).toBeA(DModel.Media);
        });
    });

    describe('JSON-LD marshaler', function () {
        it('should construct from a JSON-LD document', function () {
            expect(videoObject).toBeDefined();
        });

        it('should marshal properties', function () {
            expect(videoObject.duration).toBe(666);
            expect(videoObject.transcript).toBe('We\'re no strangers to love, etc etc etc');
            expect(videoObject.poster_uri).toBe('ekn:///89012345678901234567');
        });

        it('should inherit properties set by parent class (DModel.Media)', function () {
            expect(videoObject.caption).toBe('If this song was sushi, it would be a Rick Roll');
            expect(videoObject.width).toBe(666);
            expect(videoObject.height).toBe(666);
        });
    });
});
