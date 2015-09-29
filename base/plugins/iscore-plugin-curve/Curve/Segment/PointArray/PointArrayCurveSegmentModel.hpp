#pragma once
#include <memory>
#include <boost/container/flat_map.hpp>

#include "Curve/Segment/CurveSegmentModel.hpp"
#include "Curve/Segment/Linear/LinearCurveSegmentModel.hpp"

struct PointArrayCurveSegmentData
{
    double min_x, max_x;
    double min_y, max_y;
    QVector<QPointF> m_points;
};

Q_DECLARE_METATYPE(PointArrayCurveSegmentData)

class PointArrayCurveSegmentModel : public CurveSegmentModel
{
        Q_OBJECT
    public:
        using data_type = PointArrayCurveSegmentData;
        using CurveSegmentModel::CurveSegmentModel;        
        PointArrayCurveSegmentModel(
                const CurveSegmentData& dat,
                QObject* parent);

        template<typename Impl>
        PointArrayCurveSegmentModel(Deserializer<Impl>& vis, QObject* parent) :
            CurveSegmentModel {vis, parent}
        {
            vis.writeTo(*this);
        }

        CurveSegmentModel* clone(
                const Id<CurveSegmentModel>& id,
                QObject* parent) const override;

        QString name() const override;
        void serialize(const VisitorVariant& vis) const override;
        void on_startChanged() override;
        void on_endChanged() override;

        void updateData(int numInterp) const override;
        double valueAt(double x) const override;

        void addPoint(double, double);
        void simplify();
        std::vector<std::unique_ptr<LinearCurveSegmentModel>> piecewise() const;

        double min() { return min_y; }
        double max() { return max_y; }

    signals:
        void minChanged(double);
        void maxChanged(double);

    private:
        // Coordinates in {x, y}.
        double min_x{}, max_x{};
        double min_y{}, max_y{};

        boost::container::flat_map<double, double> m_points;
};
